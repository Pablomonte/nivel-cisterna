#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "debug.h"
#include "version.h"
#include "config_manager.h"
#include "tank.h"
#include "level_sensor.h"
#include "grafana.h"
#include "web_dashboard.h"

// ── Global objects ──────────────────────────────────────────────────
WebServer server(80);
Tank tank;
LevelSensor sensor;
GrafanaReporter grafana;
JsonDocument config;

String deviceName = "cisterna-01";

// ── WiFi ────────────────────────────────────────────────────────────

void connectWiFi() {
    String ssid = config["wifi_ssid"] | "";
    String pass = config["wifi_pass"] | "";

    if (ssid.length() == 0) {
        DBG_INFOLN("[WiFi] No SSID configured, starting AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(deviceName.c_str());
        DBG_INFO("[WiFi] AP: %s  IP: %s\n", deviceName.c_str(),
                 WiFi.softAPIP().toString().c_str());
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
        DBG_INFO("\n[WiFi] Connected! IP: %s\n",
                 WiFi.localIP().toString().c_str());
        configTime(-3 * 3600, 0, "pool.ntp.org");  // UTC-3 Argentina
    } else {
        DBG_ERRORLN("\n[WiFi] Connection failed, starting AP...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(deviceName.c_str());
        DBG_INFO("[WiFi] AP: %s  IP: %s\n", deviceName.c_str(),
                 WiFi.softAPIP().toString().c_str());
    }
}

// ── Web Handlers ────────────────────────────────────────────────────

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
    doc["uptime_sec"] = millis() / 1000;

    // WiFi info
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["free_heap"] = ESP.getFreeHeap();

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleApiConfig() {
    if (server.method() == HTTP_GET) {
        // Return current config
        String output;
        serializeJsonPretty(config, output);
        server.send(200, "application/json", output);

    } else if (server.method() == HTTP_POST) {
        // Update config
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

        config = newConfig;
        saveConfig(config);
        server.send(200, "text/plain", "Config saved. Restarting...");
        delay(500);
        ESP.restart();
    }
}

void handleRestart() {
    server.send(200, "text/plain", "Restarting...");
    delay(500);
    ESP.restart();
}

// ── Setup ───────────────────────────────────────────────────────────

void setup() {
    DEBUG_BEGIN(115200);
    delay(500);

    DBG_INFOLN("\n  NIVEL CISTERNA");
    DBG_INFO("  Firmware %s v%s\n\n", FIRMWARE_NAME, FIRMWARE_VERSION);
    DBG_INFOLN("=== SYSTEM INIT ===");

    // SPIFFS
    if (!SPIFFS.begin(true)) {
        DBG_ERRORLN("[ERR] SPIFFS mount failed");
    } else {
        DBG_INFOLN("[OK] SPIFFS mounted");
    }

    // Config
    createDefaultConfig();
    config = loadConfig();
    deviceName = config["device_name"] | "cisterna-01";

    IF_VERBOSE({
        DBG_INFOLN("\n[Config]:");
        String cfgStr;
        serializeJsonPretty(config, cfgStr);
        DBG_INFOLN(cfgStr.c_str());
    });

    // Tank geometry
    DBG_INFOLN("\n[INFO] Configuring tank...");
    JsonObject tankCfg = config["tank"];
    tank.loadFromConfig(tankCfg);
    DBG_INFO("[OK] Tank capacity: %.0f L\n", tank.getCapacity());

    // Level sensor
    DBG_INFOLN("\n[INFO] Initializing sensor...");
    JsonObject sensorCfg = config["sensor"];
    sensor.loadFromConfig(sensorCfg);
    sensor.setTank(&tank);
    if (sensor.init()) {
        DBG_INFOLN("[OK] Level sensor ready");
    } else {
        DBG_ERRORLN("[WARN] Sensor init failed - will retry");
    }

    // Grafana
    DBG_INFOLN("\n[INFO] Configuring Grafana...");
    JsonObject grafanaCfg = config["grafana"];
    grafana.loadFromConfig(grafanaCfg, deviceName.c_str());

    // WiFi
    DBG_INFOLN("\n[INFO] Connecting WiFi...");
    connectWiFi();

    // Web server
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/api/config", HTTP_POST, handleApiConfig);
    server.on("/restart", HTTP_POST, handleRestart);

    server.onNotFound([]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.enableCORS(true);
    server.begin();
    DBG_INFOLN("[OK] Web server started on port 80");

    DBG_INFOLN("\n=== SYSTEM READY ===");
    DBG_INFO("  Device: %s\n", deviceName.c_str());
    DBG_INFO("  Dashboard: http://%s/\n\n",
             WiFi.status() == WL_CONNECTED ?
             WiFi.localIP().toString().c_str() :
             WiFi.softAPIP().toString().c_str());
}

// ── Loop ────────────────────────────────────────────────────────────

void loop() {
    server.handleClient();

    // Read sensor (respects internal interval)
    if (sensor.update()) {
        // Send to Grafana
        if (grafana.shouldSend()) {
            grafana.send(sensor.getMeasurementsString());
        }
    }

    delay(10);
}
