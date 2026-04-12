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

String getWiFiPassword() {
    return config["wifi_pass"] | "";
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

    // Open AP by default; password-protected only if admin_pass is set
    bool hasPass = secrets.hasAdminPassword();
    bool started = hasPass
        ? WiFi.softAP(deviceName.c_str(), getAdminPassword().c_str())
        : WiFi.softAP(deviceName.c_str());  // open AP

    if (!started) {
        DBG_ERRORLN("[WiFi] Failed to start AP");
        return;
    }

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

    if (!wifiConfigured) {
        DBG_INFOLN("[WiFi] No SSID configured, starting secured AP");
        startAccessPoint(false);
        lastWiFiStatus = WiFi.status();
        return;
    }

    DBG_INFO("[WiFi] Connecting to %s", ssid.c_str());
    WiFi.mode(WIFI_STA);
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
        DBG_ERRORLN("\n[WiFi] Connection failed, starting AP+STA fallback");
        startAccessPoint(true);
        WiFi.begin(ssid.c_str(), pass.c_str());
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

    if (!wifiConfigured || status == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - lastWiFiReconnectAttemptMs < 30000UL) return;

    lastWiFiReconnectAttemptMs = now;
    DBG_INFOLN("[WiFi] Retrying STA connection...");
    WiFi.disconnect();
    String ssid = config["wifi_ssid"] | "";
    WiFi.begin(ssid.c_str(), getWiFiPassword().c_str());
}

bool requireAdminAuth() {
    // No auth required if no admin password has been set
    if (!secrets.hasAdminPassword()) return true;

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
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/api/config", HTTP_POST, handleApiConfig);
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
    } else {
        DBG_INFOLN("  Admin: no password (open access)");
    }
    DBG_INFO("  Dashboard: http://%s/\n\n", getActiveIpString().c_str());
}

void loop() {
    server.handleClient();
    maintainWiFi();

    sensor.update();
    pump.update(sensor.getLevel());
    telegram.update(sensor, pump);

    if (grafana.shouldSend()) {
        grafana.send(buildTelemetryFields());
    }

    delay(10);
}
