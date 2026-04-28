#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "debug.h"
#include "version.h"
#include "config_manager.h"
#include "secret_manager.h"
#include "tank.h"
#include "level_sensor.h"
#include "grafana.h"
#include "pump_controller.h"
#include "telegram_notifier.h"
#include "web_dashboard.h"

WebServer server(80);
Tank tank;
LevelSensor sensor;
GrafanaReporter grafana;
PumpController pump;
TelegramNotifier telegram;
SecretManager secrets;
JsonDocument config;

String deviceName = "cisterna-01";
String defaultAdminPassword;
bool wifiConfigured = false;
unsigned long lastWiFiReconnectAttemptMs = 0;
wl_status_t lastWiFiStatus = WL_IDLE_STATUS;

constexpr uint8_t WIFI_SCAN_MAX_RESULTS = 32;
constexpr uint32_t WIFI_SCAN_DWELL_MS = 300;
constexpr unsigned long WIFI_SCAN_TIMEOUT_MS = 15000UL;

struct WifiScanEntry {
    String ssid;
    int32_t rssi;
    int32_t channel;
    bool secure;
};

struct WifiScanState {
    bool requested;
    bool running;
    bool failed;
    bool reconnectPaused;
    uint8_t resultCount;
    unsigned long startedAtMs;
    unsigned long lastTransitionMs;
    String error;
    WifiScanEntry results[WIFI_SCAN_MAX_RESULTS];
};

WifiScanState wifiScan = {};

String deriveDefaultPassword() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[24];
    snprintf(buf, sizeof(buf), "cisterna-%06llX",
             static_cast<unsigned long long>(mac & 0xFFFFFFULL));
    return String(buf);
}

String getAdminUsername() {
    return config["admin"]["username"] | "admin";
}

String getAdminPassword() {
    String password = secrets.getAdminPassword();
    if (password.length() >= 8) {
        return password;
    }
    return defaultAdminPassword;
}

bool hasEffectiveAdminPassword() {
    return getAdminPassword().length() >= 8;
}

String getWiFiPassword() {
    return config["wifi_pass"] | "";
}

String getConnectedSsid() {
    return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
}

String trimCopy(String value) {
    value.trim();
    return value;
}

bool isWifiScanRunning() {
    return wifiScan.requested;
}

void applyStaHostname() {
    if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
        WiFi.setHostname(deviceName.c_str());
    }
}

void resetWifiScanState() {
    wifiScan.requested = false;
    wifiScan.running = false;
    wifiScan.failed = false;
    wifiScan.reconnectPaused = false;
    wifiScan.resultCount = 0;
    wifiScan.startedAtMs = 0;
    wifiScan.lastTransitionMs = 0;
    wifiScan.error = "";
    for (uint8_t i = 0; i < WIFI_SCAN_MAX_RESULTS; ++i) {
        wifiScan.results[i].ssid = "";
        wifiScan.results[i].rssi = -127;
        wifiScan.results[i].channel = 0;
        wifiScan.results[i].secure = false;
    }
}

void mergeWifiScanResult(const String& ssid, int32_t rssi, int32_t channel, bool secure) {
    if (ssid.length() == 0) return;

    for (uint8_t i = 0; i < wifiScan.resultCount; ++i) {
        if (wifiScan.results[i].ssid == ssid) {
            if (rssi > wifiScan.results[i].rssi) {
                wifiScan.results[i].rssi = rssi;
                wifiScan.results[i].channel = channel;
                wifiScan.results[i].secure = secure;
            } else if (secure) {
                wifiScan.results[i].secure = true;
            }
            return;
        }
    }

    if (wifiScan.resultCount < WIFI_SCAN_MAX_RESULTS) {
        WifiScanEntry& entry = wifiScan.results[wifiScan.resultCount++];
        entry.ssid = ssid;
        entry.rssi = rssi;
        entry.channel = channel;
        entry.secure = secure;
        return;
    }

    uint8_t weakestIndex = 0;
    for (uint8_t i = 1; i < wifiScan.resultCount; ++i) {
        if (wifiScan.results[i].rssi < wifiScan.results[weakestIndex].rssi) {
            weakestIndex = i;
        }
    }

    if (rssi > wifiScan.results[weakestIndex].rssi) {
        wifiScan.results[weakestIndex].ssid = ssid;
        wifiScan.results[weakestIndex].rssi = rssi;
        wifiScan.results[weakestIndex].channel = channel;
        wifiScan.results[weakestIndex].secure = secure;
    }
}

void pauseWiFiReconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        DBG_INFOLN("[WiFi] Reconnect pause skipped while STA is connected");
        return;
    }

    wifiScan.reconnectPaused = true;
    lastWiFiReconnectAttemptMs = millis();
    DBG_INFOLN("[WiFi] Reconnect paused for scan");
}

void resumeWiFiReconnect() {
    wifiScan.requested = false;
    wifiScan.running = false;
    wifiScan.startedAtMs = 0;

    if (!wifiScan.reconnectPaused) return;

    wifiScan.reconnectPaused = false;
    DBG_INFOLN("[WiFi] Reconnect resumed after scan");

    if (wifiConfigured && WiFi.status() != WL_CONNECTED) {
        lastWiFiReconnectAttemptMs = millis() - 30000UL;
    }
}

void failWifiScan(const String& error) {
    WiFi.scanDelete();
    wifiScan.running = false;
    wifiScan.failed = true;
    wifiScan.error = error;
    wifiScan.lastTransitionMs = millis();
    DBG_ERROR("[WiFi] Scan failed: %s\n", error.c_str());
}

void collectWiFiScanResults(int networkCount) {
    wifiScan.resultCount = 0;
    for (uint8_t i = 0; i < WIFI_SCAN_MAX_RESULTS; ++i) {
        wifiScan.results[i].ssid = "";
        wifiScan.results[i].rssi = -127;
        wifiScan.results[i].channel = 0;
        wifiScan.results[i].secure = false;
    }

    for (int i = 0; i < networkCount; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;

        mergeWifiScanResult(
            ssid,
            WiFi.RSSI(i),
            WiFi.channel(i),
            WiFi.encryptionType(i) != WIFI_AUTH_OPEN
        );
    }

    wifiScan.lastTransitionMs = millis();
    DBG_INFO("[WiFi] Scan raw=%d unique=%u\n", networkCount, wifiScan.resultCount);
}

void beginWifiScan() {
    WiFi.scanDelete();
    resetWifiScanState();
    pauseWiFiReconnect();

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
        WiFi.mode(WIFI_AP_STA);
        applyStaHostname();
        delay(100);
    }

    wifiScan.requested = true;
    wifiScan.running = true;
    wifiScan.startedAtMs = millis();
    wifiScan.lastTransitionMs = millis();

    int started = WiFi.scanNetworks(true, false, false, WIFI_SCAN_DWELL_MS, 0);
    if (started != WIFI_SCAN_RUNNING) {
        DBG_ERROR("[WiFi] scanNetworks start returned %d\n", started);
        failWifiScan("scan_start_failed");
        resumeWiFiReconnect();
        return;
    }
    DBG_INFOLN("[WiFi] Full-band scan started");
}

void cleanupTimedOutWiFiScan() {
    if (!wifiScan.requested || !wifiScan.running) return;

    if (millis() - wifiScan.startedAtMs > WIFI_SCAN_TIMEOUT_MS) {
        DBG_ERRORLN("[WiFi] Scan timeout reached");
        failWifiScan("timeout");
        resumeWiFiReconnect();
    }
}

void syncClock() {
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

String getActiveIpString() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

const char* getWifiModeString() {
    switch (WiFi.getMode()) {
        case WIFI_STA: return "sta";
        case WIFI_AP: return "ap";
        case WIFI_AP_STA: return "ap_sta";
        default: return "off";
    }
}

void startAccessPoint(bool withStation) {
    wifi_mode_t mode = withStation ? WIFI_AP_STA : WIFI_AP;
    WiFi.mode(mode);

    bool hasPass = hasEffectiveAdminPassword();
    bool started = hasPass
        ? WiFi.softAP(deviceName.c_str(), getAdminPassword().c_str())
        : WiFi.softAP(deviceName.c_str());

    if (!started) {
        DBG_ERRORLN("[WiFi] Failed to start AP");
        return;
    }

    WiFi.softAPsetHostname(deviceName.c_str());

    DBG_INFO("[WiFi] AP: %s  IP: %s  %s\n",
             deviceName.c_str(),
             WiFi.softAPIP().toString().c_str(),
             hasPass ? "(password protected)" : "(open)");
}

void connectWiFi() {
    String ssid = config["wifi_ssid"] | "";
    String pass = getWiFiPassword();

    wifiConfigured = ssid.length() > 0;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);

    // AP siempre activo (modo AP_STA) para acceso al panel aunque STA conecte
    startAccessPoint(true);
    applyStaHostname();

    if (!wifiConfigured) {
        DBG_INFOLN("[WiFi] No SSID configured, AP-only access");
        lastWiFiStatus = WiFi.status();
        return;
    }

    DBG_INFO("[WiFi] Connecting to %s", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        DBG_INFO(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DBG_INFO("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        syncClock();
    } else {
        DBG_ERRORLN("\n[WiFi] STA connect failed, AP+STA fallback continues");
    }

    lastWiFiStatus = WiFi.status();
}

void maintainWiFi() {
    wl_status_t status = WiFi.status();
    if (status != lastWiFiStatus) {
        if (status == WL_CONNECTED) {
            DBG_INFO("[WiFi] Reconnected. IP: %s\n", WiFi.localIP().toString().c_str());
            syncClock();
        } else {
            DBG_ERROR("[WiFi] Status changed: %d\n", static_cast<int>(status));
        }
        lastWiFiStatus = status;
    }

    if (!wifiConfigured || status == WL_CONNECTED || wifiScan.reconnectPaused || isWifiScanRunning()) return;

    unsigned long now = millis();
    if (now - lastWiFiReconnectAttemptMs < 30000UL) return;

    lastWiFiReconnectAttemptMs = now;
    DBG_INFOLN("[WiFi] Retrying STA connection...");
    WiFi.disconnect();
    String ssid = config["wifi_ssid"] | "";
    WiFi.begin(ssid.c_str(), getWiFiPassword().c_str());
}

bool requireAdminAuth() {
    if (!hasEffectiveAdminPassword()) return true;

    String username = getAdminUsername();
    String password = getAdminPassword();

    if (!server.authenticate(username.c_str(), password.c_str())) {
        server.requestAuthentication(BASIC_AUTH, "nivel-cisterna", "Authentication required");
        return false;
    }
    return true;
}

String buildTelemetryFields() {
    String fields = sensor.getMeasurementsString();
    fields.reserve(320);
    fields += ",capacity=" + String(tank.getCapacity(), 1);
    fields += ",wifi_connected=" + String(WiFi.status() == WL_CONNECTED ? 1 : 0) + "i";
    fields += ",free_heap=" + String(ESP.getFreeHeap()) + "i";
    fields += ",uptime_sec=" + String(millis() / 1000UL) + "i";
    fields += ",";
    fields += pump.getGrafanaFields();
    return fields;
}

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleWifiPage() {
    if (!requireAdminAuth()) return;
    server.send_P(200, "text/html", WIFI_ADMIN_HTML);
}

void handleApiStatus() {
    JsonDocument doc;

    doc["device"] = deviceName;
    doc["version"] = FIRMWARE_VERSION;
    doc["level"] = sensor.getLevel();
    doc["distance"] = sensor.getDistance();
    doc["volume"] = sensor.getVolume();
    doc["capacity"] = tank.getCapacity();
    doc["sensor_ok"] = sensor.isOk();
    doc["sensor_failures"] = sensor.getConsecutiveFailures();
    doc["sensor_spread_cm"] = sensor.getSpreadCm();
    doc["last_success_age_sec"] = sensor.getLastSuccessAgeSec();
    doc["uptime_sec"] = millis() / 1000UL;

    doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi_rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    doc["wifi_mode"] = getWifiModeString();
    doc["wifi_ssid"] = config["wifi_ssid"] | "";
    doc["connected_ssid"] = getConnectedSsid();
    doc["ap_ssid"] = deviceName;
    doc["ip"] = getActiveIpString();
    doc["free_heap"] = ESP.getFreeHeap();

    doc["pump_enabled"] = pump.isEnabled();
    doc["pump_on"] = pump.isOn();
    doc["pump_state"] = pump.getStateString();
    doc["pump_state_code"] = pump.getStateCode();
    doc["pump_auto_mode"] = pump.isAutoMode();
    doc["pump_runtime_sec"] = pump.getTotalRuntimeSec();

    doc["grafana_configured"] = grafana.isConfigured();
    doc["grafana_last_http_code"] = grafana.getLastHttpCode();
    doc["grafana_last_success_age_sec"] = grafana.getLastSuccessAgeSec();
    doc["telegram_configured"] = telegram.isConfigured();

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleApiWifiSettings() {
    if (!requireAdminAuth()) return;

    if (server.method() == HTTP_GET) {
        JsonDocument doc;
        doc["device_name"] = config["device_name"] | deviceName;
        doc["ap_ssid"] = deviceName;
        doc["wifi_ssid"] = config["wifi_ssid"] | "";
        doc["wifi_pass_configured"] = getWiFiPassword().length() > 0;
        doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
        doc["connected_ssid"] = getConnectedSsid();
        doc["wifi_mode"] = getWifiModeString();
        doc["ip"] = getActiveIpString();

        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "No body");
        return;
    }

    JsonDocument patchDoc;
    DeserializationError err = deserializeJson(patchDoc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", String("JSON error: ") + err.c_str());
        return;
    }

    JsonObject patch = patchDoc.as<JsonObject>();
    if (patch.isNull()) {
        server.send(400, "text/plain", "Body must be a JSON object");
        return;
    }

    JsonDocument updatedConfig = config;

    if (!patch["device_name"].isNull()) {
        updatedConfig["device_name"] = trimCopy(patch["device_name"].as<String>());
    }
    if (!patch["wifi_ssid"].isNull()) {
        updatedConfig["wifi_ssid"] = trimCopy(patch["wifi_ssid"].as<String>());
    }
    if (!patch["wifi_pass"].isNull()) {
        updatedConfig["wifi_pass"] = patch["wifi_pass"] | "";
    }

    applyConfigDefaults(updatedConfig);

    String validationError;
    if (!validateConfig(updatedConfig, validationError)) {
        server.send(400, "text/plain", validationError);
        return;
    }

    secrets.absorb(updatedConfig);
    if (!saveConfig(updatedConfig)) {
        server.send(500, "text/plain", "Failed to save WiFi settings");
        return;
    }

    server.send(200, "text/plain", "Configuracion WiFi guardada. Reiniciando...");
    delay(500);
    ESP.restart();
}

void handleApiWifiScan() {
    if (!requireAdminAuth()) return;

    JsonDocument doc;

    if (server.method() == HTTP_POST) {
        if (wifiScan.requested) {
            doc["status"] = "running";
            doc["started_ms_ago"] = millis() - wifiScan.startedAtMs;
            String output;
            serializeJson(doc, output);
            server.send(202, "application/json", output);
            return;
        }

        beginWifiScan();
        if (wifiScan.failed || !wifiScan.requested) {
            doc["status"] = "failed";
            doc["error"] = wifiScan.error.length() > 0 ? wifiScan.error : "scan_start_failed";
            String output;
            serializeJson(doc, output);
            server.send(200, "application/json", output);
            return;
        }

        doc["status"] = "running";
        doc["started_ms_ago"] = 0;
        String output;
        serializeJson(doc, output);
        server.send(202, "application/json", output);
        return;
    }

    if (!wifiScan.requested) {
        if (wifiScan.failed) {
            doc["status"] = "failed";
            doc["error"] = wifiScan.error;
            String output;
            serializeJson(doc, output);
            server.send(200, "application/json", output);
            return;
        }

        doc["status"] = "idle";
        doc["count"] = 0;
        doc["networks"].to<JsonArray>();
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING) {
        if (millis() - wifiScan.startedAtMs > WIFI_SCAN_TIMEOUT_MS) {
            failWifiScan("timeout");
            resumeWiFiReconnect();
            doc["status"] = "failed";
            doc["error"] = "timeout";
            String output;
            serializeJson(doc, output);
            server.send(200, "application/json", output);
            return;
        }

        doc["status"] = "running";
        doc["started_ms_ago"] = millis() - wifiScan.startedAtMs;
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    if (result < 0) {
        failWifiScan("scan_failed");
        resumeWiFiReconnect();
        doc["status"] = "failed";
        doc["error"] = wifiScan.error;
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    collectWiFiScanResults(result);
    WiFi.scanDelete();
    resumeWiFiReconnect();

    String configuredSsid = config["wifi_ssid"] | "";
    String connectedSsid = getConnectedSsid();
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (uint8_t i = 0; i < wifiScan.resultCount; ++i) {
        const WifiScanEntry& entry = wifiScan.results[i];
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = entry.ssid;
        network["rssi"] = entry.rssi;
        network["channel"] = entry.channel;
        network["secure"] = entry.secure;
        network["configured"] = entry.ssid == configuredSsid;
        network["connected"] = entry.ssid == connectedSsid;
    }

    doc["status"] = "complete";
    doc["count"] = networks.size();
    doc["duration_ms"] = wifiScan.lastTransitionMs - wifiScan.startedAtMs;

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleApiConfig() {
    if (!requireAdminAuth()) return;

    if (server.method() == HTTP_GET) {
        JsonDocument redacted = config;
        secrets.redact(redacted);

        String output;
        serializeJsonPretty(redacted, output);
        server.send(200, "application/json", output);
        return;
    }

    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "No body");
        return;
    }

    JsonDocument newConfig;
    DeserializationError err = deserializeJson(newConfig, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", String("JSON error: ") + err.c_str());
        return;
    }

    applyConfigDefaults(newConfig);

    JsonDocument validationConfig = newConfig;
    secrets.apply(validationConfig);

    String validationError;
    if (!validateConfig(validationConfig, validationError)) {
        server.send(400, "text/plain", validationError);
        return;
    }

    secrets.absorb(newConfig);
    if (!saveConfig(newConfig)) {
        server.send(500, "text/plain", "Failed to save config");
        return;
    }

    server.send(200, "text/plain", "Config saved. Restarting...");
    delay(500);
    ESP.restart();
}

void handleApiPump() {
    if (!requireAdminAuth()) return;

    if (!pump.isEnabled()) {
        server.send(409, "text/plain", "Pump controller disabled");
        return;
    }

    String action = server.arg("action");
    if (action == "on") {
        pump.manualOn();
    } else if (action == "off") {
        pump.manualOff();
    } else if (action == "auto") {
        pump.resetToAuto();
    } else {
        server.send(400, "text/plain", "Invalid action");
        return;
    }

    server.send(200, "text/plain", String("Pump action applied: ") + action);
}

void handleRestart() {
    if (!requireAdminAuth()) return;

    server.send(200, "text/plain", "Restarting...");
    delay(500);
    ESP.restart();
}

void loadRuntimeConfig() {
    createDefaultConfig();
    config = loadConfig();
    applyConfigDefaults(config);
    secrets.apply(config);

    String validationError;
    if (!validateConfig(config, validationError)) {
        DBG_ERROR("[Config] Invalid configuration: %s\n", validationError.c_str());

        JsonDocument fallback;
        applyConfigDefaults(fallback);
        secrets.apply(fallback);
        config = fallback;
    }
}

void setup() {
    DEBUG_BEGIN(115200);
    delay(500);

    DBG_INFOLN("\n  NIVEL CISTERNA");
    DBG_INFO("  Firmware %s v%s\n\n", FIRMWARE_NAME, FIRMWARE_VERSION);
    DBG_INFOLN("=== SYSTEM INIT ===");

    defaultAdminPassword = deriveDefaultPassword();

    if (!SPIFFS.begin(true)) {
        DBG_ERRORLN("[ERR] SPIFFS mount failed");
    } else {
        DBG_INFOLN("[OK] SPIFFS mounted");
    }

    secrets.begin();
    loadRuntimeConfig();

    deviceName = config["device_name"] | "cisterna-01";

    IF_VERBOSE({
        JsonDocument redacted = config;
        secrets.redact(redacted);
        DBG_INFOLN("\n[Config]:");
        String cfgStr;
        serializeJsonPretty(redacted, cfgStr);
        DBG_INFOLN(cfgStr.c_str());
    });

    DBG_INFOLN("\n[INFO] Configuring tank...");
    tank.loadFromConfig(config["tank"].as<JsonObject>());
    DBG_INFO("[OK] Tank capacity: %.0f L\n", tank.getCapacity());

    DBG_INFOLN("\n[INFO] Initializing sensor...");
    sensor.loadFromConfig(config["sensor"].as<JsonObject>());
    sensor.setTank(&tank);
    if (sensor.init()) {
        DBG_INFOLN("[OK] Level sensor ready");
    } else {
        DBG_ERRORLN("[WARN] Sensor init failed - will retry");
    }

    DBG_INFOLN("\n[INFO] Configuring pump...");
    pump.loadFromConfig(config["pump"].as<JsonObject>());
    if (pump.init()) {
        DBG_INFOLN("[OK] Pump controller ready");
    } else {
        DBG_INFOLN("[Pump] Disabled");
    }

    DBG_INFOLN("\n[INFO] Configuring Grafana...");
    grafana.loadFromConfig(config["grafana"].as<JsonObject>(), deviceName.c_str());

    DBG_INFOLN("\n[INFO] Configuring Telegram...");
    telegram.loadFromConfig(config["telegram"].as<JsonObject>(), deviceName.c_str());

    DBG_INFOLN("\n[INFO] Connecting WiFi...");
    connectWiFi();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiPage);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/api/config", HTTP_POST, handleApiConfig);
    server.on("/api/wifi/settings", HTTP_GET, handleApiWifiSettings);
    server.on("/api/wifi/settings", HTTP_POST, handleApiWifiSettings);
    server.on("/api/wifi/scan", HTTP_GET, handleApiWifiScan);
    server.on("/api/wifi/scan", HTTP_POST, handleApiWifiScan);
    server.on("/api/pump", HTTP_POST, handleApiPump);
    server.on("/restart", HTTP_POST, handleRestart);

    server.onNotFound([]() {
        if (server.uri().startsWith("/api/")) {
            server.send(404, "application/json", "{\"error\":\"not_found\"}");
            return;
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
    DBG_INFOLN("[OK] Web server started on port 80");

    DBG_INFOLN("\n=== SYSTEM READY ===");
    DBG_INFO("  Device: %s\n", deviceName.c_str());
    if (secrets.hasAdminPassword()) {
        DBG_INFO("  Admin: %s / (configured)\n", getAdminUsername().c_str());
    } else if (hasEffectiveAdminPassword()) {
        DBG_INFO("  Admin: %s / %s (default)\n",
                 getAdminUsername().c_str(),
                 getAdminPassword().c_str());
    } else {
        DBG_INFOLN("  Admin: no password (open access)");
    }
    DBG_INFO("  Dashboard: http://%s/\n\n", getActiveIpString().c_str());
}

void loop() {
    server.handleClient();
    cleanupTimedOutWiFiScan();
    maintainWiFi();

    sensor.update();
    pump.update(sensor.getLevel());
    telegram.update(sensor, pump);

    if (grafana.shouldSend()) {
        grafana.send(buildTelemetryFields());
    }

    delay(10);
}
