#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "debug.h"
#include "version.h"
#include "config_manager.h"
#include "secret_manager.h"
#include "tank.h"
#include "level_sensor.h"
#include "grafana.h"
#include "web_dashboard.h"
#include "power_manager.h"

WebServer server(80);
DNSServer dnsServer;
bool dnsServerActive = false;
Tank tank;
LevelSensor sensor;
GrafanaReporter grafana;
PowerManager powerManager;
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

// Guard defensivo contra escrituras de config solapadas. WebServer es
// single-threaded, asi que en condiciones normales esto nunca se gatilla;
// sirve por si algun handler cede tiempo (yield, SPIFFS lento) y otra
// request reentrante intenta escribir simultaneamente.
volatile bool configWriteBusy = false;

struct ConfigWriteGuard {
    bool acquired;
    ConfigWriteGuard() : acquired(false) {
        if (!configWriteBusy) {
            configWriteBusy = true;
            acquired = true;
        }
    }
    ~ConfigWriteGuard() {
        if (acquired) configWriteBusy = false;
    }
};

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

    // Configurar AP explicitamente: IP, gateway y subnet identicos a la IP del
    // AP. Esto garantiza que el servidor DHCP integrado anuncie al propio ESP
    // como DNS server (necesario para que el portal cautivo intercepte las
    // consultas DNS de los clientes que se asocian al AP).
    IPAddress apIp(192, 168, 4, 1);
    IPAddress apMask(255, 255, 255, 0);
    if (!WiFi.softAPConfig(apIp, apIp, apMask)) {
        DBG_ERRORLN("[WiFi] softAPConfig failed");
    }

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

    if (!dnsServerActive) {
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.setTTL(60);
        if (dnsServer.start(53, "*", WiFi.softAPIP())) {
            dnsServerActive = true;
            DBG_INFO("[WiFi] Captive DNS server up on %s:53 (wildcard)\n",
                     WiFi.softAPIP().toString().c_str());
        } else {
            DBG_ERRORLN("[WiFi] Failed to start captive DNS server");
        }
    }
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
    if (now - lastWiFiReconnectAttemptMs < powerManager.getWifiRetryIntervalMs()) return;

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
    doc["tank_calibrated"] = tank.isCalibrated();
    doc["empty_distance_cm"] = tank.getEmptyDistance();
    doc["full_distance_cm"] = tank.getFullDistance();
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

    doc["grafana_configured"] = grafana.isConfigured();
    doc["grafana_last_http_code"] = grafana.getLastHttpCode();
    doc["grafana_last_success_age_sec"] = grafana.getLastSuccessAgeSec();

    doc["power_mode"] = powerManager.isBatteryMode() ? "battery" : "normal";
    doc["boot_count"] = powerManager.getBootCount();

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
        doc["admin_password_configured"] = secrets.hasAdminPassword();
        doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
        doc["connected_ssid"] = getConnectedSsid();
        doc["wifi_mode"] = getWifiModeString();
        doc["ip"] = getActiveIpString();

        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    ConfigWriteGuard guard;
    if (!guard.acquired) {
        server.send(409, "application/json", "{\"error\":\"busy_try_again\"}");
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
        // Password vacio explicito = red abierta detectada por el frontend.
        // Si el campo no viene en el patch, se conserva la actual.
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

void handleApiAdminPassword() {
    if (!requireAdminAuth()) return;

    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method not allowed");
        return;
    }

    ConfigWriteGuard guard;
    if (!guard.acquired) {
        server.send(409, "application/json", "{\"error\":\"busy_try_again\"}");
        return;
    }

    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "No body");
        return;
    }

    JsonDocument body;
    DeserializationError err = deserializeJson(body, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", String("JSON error: ") + err.c_str());
        return;
    }

    String current = body["current"] | "";
    String next = body["new"] | "";

    if (next.length() < 8) {
        server.send(400, "text/plain", "new password must be at least 8 characters");
        return;
    }
    if (next.length() > 64) {
        server.send(400, "text/plain", "new password must be 64 characters or fewer");
        return;
    }

    if (secrets.hasAdminPassword()) {
        if (current != secrets.getAdminPassword()) {
            server.send(401, "text/plain", "current password mismatch");
            return;
        }
    }

    if (next == secrets.getAdminPassword()) {
        server.send(400, "text/plain", "new password must differ from current");
        return;
    }

    JsonDocument secretDoc;
    JsonObject admin = secretDoc["admin"].to<JsonObject>();
    admin["password"] = next;
    secrets.absorb(secretDoc);

    server.send(200, "text/plain", "Admin password updated");
}

void handleApiSensorCalibrate() {
    if (!requireAdminAuth()) return;

    if (server.method() == HTTP_GET) {
        JsonDocument doc;
        doc["empty_distance_cm"] = tank.getEmptyDistance();
        doc["full_distance_cm"] = tank.getFullDistance();
        doc["offset_cm"] = sensor.getOffsetCm();
        doc["current_distance_cm"] = sensor.getDistance();
        doc["current_level_pct"] = sensor.getLevel();
        doc["sensor_ok"] = sensor.isOk();
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    ConfigWriteGuard guard;
    if (!guard.acquired) {
        server.send(409, "application/json", "{\"error\":\"busy_try_again\"}");
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

    JsonDocument updatedConfig;
    updatedConfig.set(config);

    JsonObject configTank = updatedConfig["tank"].is<JsonObject>()
        ? updatedConfig["tank"].as<JsonObject>()
        : updatedConfig["tank"].to<JsonObject>();
    JsonObject configSensor = updatedConfig["sensor"].is<JsonObject>()
        ? updatedConfig["sensor"].as<JsonObject>()
        : updatedConfig["sensor"].to<JsonObject>();

    JsonObject patchTank = patch["tank"].as<JsonObject>();
    if (!patchTank.isNull()) {
        if (!patchTank["empty_distance_cm"].isNull()) {
            configTank["empty_distance_cm"] = patchTank["empty_distance_cm"].as<float>();
        }
        if (!patchTank["full_distance_cm"].isNull()) {
            configTank["full_distance_cm"] = patchTank["full_distance_cm"].as<float>();
        }
    }

    JsonObject patchSensor = patch["sensor"].as<JsonObject>();
    if (!patchSensor.isNull() && !patchSensor["offset_cm"].isNull()) {
        configSensor["offset_cm"] = patchSensor["offset_cm"].as<float>();
    }

    applyConfigDefaults(updatedConfig);

    String validationError;
    if (!validateConfig(updatedConfig, validationError)) {
        server.send(400, "text/plain", validationError);
        return;
    }

    secrets.absorb(updatedConfig);
    if (!saveConfig(updatedConfig)) {
        server.send(500, "text/plain", "Failed to save calibration");
        return;
    }

    config = updatedConfig;
    tank.loadFromConfig(config["tank"].as<JsonObject>());
    sensor.loadFromConfig(config["sensor"].as<JsonObject>());

    server.send(200, "text/plain", "Calibracion guardada y aplicada.");
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

void handleApiPower() {
    if (!requireAdminAuth()) return;

    if (server.method() == HTTP_GET) {
        JsonDocument doc;
        doc["mode"]                    = config["power"]["mode"]                    | "normal";
        doc["sleep_interval_sec"]      = config["power"]["sleep_interval_sec"]      | 300;
        doc["web_window_sec"]          = config["power"]["web_window_sec"]          | 60;
        doc["wifi_timeout_ms"]         = config["power"]["wifi_timeout_ms"]         | 20000;
        doc["wifi_retry_interval_sec"] = config["power"]["wifi_retry_interval_sec"] | 30;
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
        return;
    }

    ConfigWriteGuard guard;
    if (!guard.acquired) {
        server.send(409, "application/json", "{\"error\":\"busy_try_again\"}");
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

    JsonDocument updatedConfig = config;
    if (!patchDoc["mode"].isNull())
        updatedConfig["power"]["mode"] = patchDoc["mode"].as<String>();
    if (!patchDoc["sleep_interval_sec"].isNull())
        updatedConfig["power"]["sleep_interval_sec"] = patchDoc["sleep_interval_sec"].as<int>();
    if (!patchDoc["web_window_sec"].isNull())
        updatedConfig["power"]["web_window_sec"] = patchDoc["web_window_sec"].as<int>();
    if (!patchDoc["wifi_timeout_ms"].isNull())
        updatedConfig["power"]["wifi_timeout_ms"] = patchDoc["wifi_timeout_ms"].as<int>();
    if (!patchDoc["wifi_retry_interval_sec"].isNull())
        updatedConfig["power"]["wifi_retry_interval_sec"] = patchDoc["wifi_retry_interval_sec"].as<int>();

    applyConfigDefaults(updatedConfig);

    String validationError;
    if (!validateConfig(updatedConfig, validationError)) {
        server.send(400, "text/plain", validationError);
        return;
    }

    secrets.absorb(updatedConfig);
    if (!saveConfig(updatedConfig)) {
        server.send(500, "text/plain", "Failed to save config");
        return;
    }

    server.send(200, "text/plain", "Configuracion de energia guardada. Reiniciando...");
    delay(500);
    ESP.restart();
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

    ConfigWriteGuard guard;
    if (!guard.acquired) {
        server.send(409, "application/json", "{\"error\":\"busy_try_again\"}");
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

void setupOTA() {
    ArduinoOTA.setHostname(deviceName.c_str());

    String otaPwd = getAdminPassword();
    if (otaPwd.length() > 0) {
        ArduinoOTA.setPassword(otaPwd.c_str());
    }

    ArduinoOTA.onStart([]() {
        const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        DBG_INFO("[OTA] Update starting (%s)\n", type);
    });
    ArduinoOTA.onEnd([]() {
        DBG_INFOLN("[OTA] Update complete, rebooting");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total == 0) return;
        DBG_INFO("[OTA] %u%%\r", (progress * 100U) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        const char* msg = "unknown";
        switch (error) {
            case OTA_AUTH_ERROR:    msg = "auth failed"; break;
            case OTA_BEGIN_ERROR:   msg = "begin failed"; break;
            case OTA_CONNECT_ERROR: msg = "connect failed"; break;
            case OTA_RECEIVE_ERROR: msg = "receive failed"; break;
            case OTA_END_ERROR:     msg = "end failed"; break;
        }
        DBG_ERROR("[OTA] Error: %s\n", msg);
    });

    ArduinoOTA.begin();
    DBG_INFO("[OTA] Listening as %s on port 3232 (auth: %s)\n",
             deviceName.c_str(),
             otaPwd.length() > 0 ? "yes" : "no");
}

void handleCaptivePortalProbe() {
    // Los SO consultan endpoints conocidos para detectar portal cautivo:
    //  - Android: /generate_204 espera 204. Cualquier otra cosa dispara la
    //    notificacion "Iniciar sesion en la red" y autoabre el navegador.
    //  - iOS: /hotspot-detect.html y /library/test/success.html esperan HTML
    //    con la palabra "Success". Si no aparece, se abre el sheet del portal.
    //  - Windows: /connecttest.txt y /ncsi.txt esperan contenido especifico.
    // Servimos una pagina HTML minima con meta-refresh + JS redirect al
    // dashboard. NO contiene "Success" para iOS y NO retorna 204 para Android.
    DBG_INFO("[CAPTIVE] probe hit: %s\n", server.uri().c_str());
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    String apIp = WiFi.softAPIP().toString();
    String redirectUrl = "http://" + apIp + "/";
    String body = "<!DOCTYPE html><html><head>"
        "<meta http-equiv=\"refresh\" content=\"0; url=" + redirectUrl + "\">"
        "<title>Cisterna</title></head>"
        "<body><script>location.href='" + redirectUrl + "';</script>"
        "<a href=\"" + redirectUrl + "\">Abrir panel del dispositivo</a>"
        "</body></html>";
    server.send(200, "text/html", body);
}

void registerCaptivePortalProbes() {
    static const char* const PROBE_PATHS[] = {
        "/generate_204",
        "/gen_204",
        "/hotspot-detect.html",
        "/library/test/success.html",
        "/connecttest.txt",
        "/ncsi.txt",
        "/redirect",
        "/canonical.html",
        "/success.txt",
        "/check_network_status.txt",
        "/mobile/status.php",
        "/fwlink"
    };
    for (auto path : PROBE_PATHS) {
        server.on(path, HTTP_GET, handleCaptivePortalProbe);
    }
}

void registerRoutes() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiPage);
    server.on("/config", HTTP_GET, handleWifiPage);
    registerCaptivePortalProbes();
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/api/config", HTTP_POST, handleApiConfig);
    server.on("/api/wifi/settings", HTTP_GET, handleApiWifiSettings);
    server.on("/api/wifi/settings", HTTP_POST, handleApiWifiSettings);
    server.on("/api/wifi/scan", HTTP_GET, handleApiWifiScan);
    server.on("/api/wifi/scan", HTTP_POST, handleApiWifiScan);
    server.on("/api/admin/password", HTTP_POST, handleApiAdminPassword);
    server.on("/api/sensor/calibrate", HTTP_GET, handleApiSensorCalibrate);
    server.on("/api/sensor/calibrate", HTTP_POST, handleApiSensorCalibrate);
    server.on("/api/power", HTTP_GET, handleApiPower);
    server.on("/api/power", HTTP_POST, handleApiPower);
    server.on("/restart", HTTP_POST, handleRestart);

    server.onNotFound([]() {
        if (server.uri().startsWith("/api/")) {
            server.send(404, "application/json", "{\"error\":\"not_found\"}");
            return;
        }
        // Cualquier URL desconocida cuenta como probe del portal cautivo.
        DBG_INFO("[CAPTIVE] notFound: host=%s uri=%s\n",
                 server.hostHeader().c_str(),
                 server.uri().c_str());
        handleCaptivePortalProbe();
    });
}

void runBatteryModeCycle() {
    sensor.resetReadTimer();
    sensor.update();
    rtcState.lastLevel = sensor.getLevel();

    bool wifiOk = powerManager.connectWithTimeout(
        config["wifi_ssid"] | "",
        getWiFiPassword()
    );

    if (wifiOk) {
        syncClock();
        grafana.send(buildTelemetryFields());
    }

    startAccessPoint(wifiOk);
    setupOTA();
    registerRoutes();
    server.begin();
    DBG_INFOLN("[OK] Web server started");

    powerManager.startWebWindow();
    while (powerManager.isWebWindowActive()) {
        ArduinoOTA.handle();
        if (dnsServerActive) {
            dnsServer.processNextRequest();
        }
        server.handleClient();
        sensor.update();
        delay(10);
    }

    if (dnsServerActive) {
        dnsServer.stop();
        dnsServerActive = false;
    }
    powerManager.enterDeepSleep();
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

    powerManager.loadFromConfig(config["power"].as<JsonObject>());

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

    DBG_INFOLN("\n[INFO] Configuring Grafana...");
    grafana.loadFromConfig(config["grafana"].as<JsonObject>(), deviceName.c_str());

    if (powerManager.isBatteryMode()) {
        DBG_INFOLN("\n[Power] Battery mode - single cycle");
        runBatteryModeCycle();
        // no retorna
    }

    DBG_INFOLN("\n[INFO] Connecting WiFi...");
    connectWiFi();

    WiFi.setSleep(WIFI_PS_NONE);

    setupOTA();

    registerRoutes();

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
    ArduinoOTA.handle();
    if (dnsServerActive) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
    cleanupTimedOutWiFiScan();
    maintainWiFi();

    sensor.update();

    if (grafana.shouldSend()) {
        grafana.send(buildTelemetryFields());
    }

    delay(10);
}
