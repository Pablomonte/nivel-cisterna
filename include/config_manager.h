#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "debug.h"

#define CONFIG_FILE "/config.json"
#define CONFIG_TMP_FILE "/config.tmp"

inline void applyConfigDefaults(JsonDocument& doc) {
    doc["device_name"] = doc["device_name"] | "cisterna-01";
    doc["wifi_ssid"] = doc["wifi_ssid"] | "";
    doc["wifi_pass"] = doc["wifi_pass"] | "";

    JsonObject sensor = doc["sensor"];
    if (sensor.isNull()) sensor = doc["sensor"].to<JsonObject>();
    sensor["trigger_pin"] = sensor["trigger_pin"] | 5;
    sensor["echo_pin"] = sensor["echo_pin"] | 18;
    sensor["samples"] = sensor["samples"] | 5;
    sensor["read_interval_sec"] = sensor["read_interval_sec"] | 10;
    sensor["offset_cm"] = sensor["offset_cm"] | 0.0f;

    JsonObject tank = doc["tank"];
    if (tank.isNull()) tank = doc["tank"].to<JsonObject>();
    tank["shape"] = tank["shape"] | "cylindrical";
    tank["height_cm"] = tank["height_cm"] | 150.0f;
    tank["diameter_cm"] = tank["diameter_cm"] | 120.0f;
    tank["width_cm"] = tank["width_cm"] | 0.0f;
    tank["length_cm"] = tank["length_cm"] | 0.0f;
    tank["capacity_liters"] = tank["capacity_liters"] | 0.0f;
    tank["empty_distance_cm"] = tank["empty_distance_cm"] | 145.0f;
    tank["full_distance_cm"] = tank["full_distance_cm"] | 10.0f;

    JsonObject grafana = doc["grafana"];
    if (grafana.isNull()) grafana = doc["grafana"].to<JsonObject>();
    grafana["url"] = grafana["url"] | "";
    grafana["token"] = grafana["token"] | "";
    grafana["send_interval_sec"] = grafana["send_interval_sec"] | 10;
    grafana["timeout_ms"] = grafana["timeout_ms"] | 5000;

    JsonObject admin = doc["admin"];
    if (admin.isNull()) admin = doc["admin"].to<JsonObject>();
    admin["username"] = admin["username"] | "admin";
    admin["password"] = admin["password"] | "";

    JsonObject power = doc["power"];
    if (power.isNull()) power = doc["power"].to<JsonObject>();
    power["mode"]                    = power["mode"]                    | "normal";
    power["sleep_interval_sec"]      = power["sleep_interval_sec"]      | 300;
    power["web_window_sec"]          = power["web_window_sec"]          | 60;
    power["wifi_timeout_ms"]         = power["wifi_timeout_ms"]         | 20000;
    power["wifi_retry_interval_sec"] = power["wifi_retry_interval_sec"] | 30;
}

inline bool validateConfig(const JsonDocument& doc, String& error) {
    String deviceName = doc["device_name"] | "";
    if (deviceName.length() == 0) {
        error = "device_name is required";
        return false;
    }
    if (deviceName.length() > 31) {
        error = "device_name must be 31 characters or fewer";
        return false;
    }

    String wifiSsid = doc["wifi_ssid"] | "";
    if (wifiSsid.length() > 32) {
        error = "wifi_ssid must be 32 characters or fewer";
        return false;
    }

    String wifiPass = doc["wifi_pass"] | "";
    if (wifiPass.length() > 0 && (wifiPass.length() < 8 || wifiPass.length() > 63)) {
        error = "wifi_pass must be 8-63 characters (WPA2) or empty for AP-only";
        return false;
    }

    JsonObjectConst sensor = doc["sensor"].as<JsonObjectConst>();
    int samples = sensor["samples"] | 5;
    int readIntervalSec = sensor["read_interval_sec"] | 10;
    float offsetCm = sensor["offset_cm"] | 0.0f;
    if (samples < 1 || samples > 15) {
        error = "sensor.samples must be between 1 and 15";
        return false;
    }
    if (readIntervalSec < 1) {
        error = "sensor.read_interval_sec must be at least 1";
        return false;
    }
    if (offsetCm < -50.0f || offsetCm > 50.0f) {
        error = "sensor.offset_cm must be between -50 and 50";
        return false;
    }

    JsonObjectConst tank = doc["tank"].as<JsonObjectConst>();
    String shape = tank["shape"] | "cylindrical";
    float heightCm = tank["height_cm"] | 150.0f;
    float diameterCm = tank["diameter_cm"] | 120.0f;
    float widthCm = tank["width_cm"] | 0.0f;
    float lengthCm = tank["length_cm"] | 0.0f;
    float capacityLiters = tank["capacity_liters"] | 0.0f;
    float emptyDistanceCm = tank["empty_distance_cm"] | 145.0f;
    float fullDistanceCm = tank["full_distance_cm"] | 10.0f;

    if (shape != "cylindrical" && shape != "rectangular") {
        error = "tank.shape must be cylindrical or rectangular";
        return false;
    }
    if (heightCm <= 0) {
        error = "tank.height_cm must be greater than 0";
        return false;
    }
    if (capacityLiters <= 0) {
        if (shape == "cylindrical" && diameterCm <= 0) {
            error = "tank.diameter_cm must be greater than 0";
            return false;
        }
        if (shape == "rectangular" && (widthCm <= 0 || lengthCm <= 0)) {
            error = "tank.width_cm and tank.length_cm must be greater than 0";
            return false;
        }
    }
    if (emptyDistanceCm <= fullDistanceCm) {
        error = "tank.empty_distance_cm must be greater than tank.full_distance_cm";
        return false;
    }

    JsonObjectConst grafana = doc["grafana"].as<JsonObjectConst>();
    int grafanaIntervalSec = grafana["send_interval_sec"] | 10;
    int grafanaTimeoutMs = grafana["timeout_ms"] | 5000;
    if (grafanaIntervalSec < 1) {
        error = "grafana.send_interval_sec must be at least 1";
        return false;
    }
    if (grafanaTimeoutMs < 1000) {
        error = "grafana.timeout_ms must be at least 1000";
        return false;
    }

    JsonObjectConst admin = doc["admin"].as<JsonObjectConst>();
    String adminUser = admin["username"] | "";
    String adminPass = admin["password"] | "";
    if (adminUser.length() == 0) {
        error = "admin.username is required";
        return false;
    }
    if (adminPass.length() > 0 && adminPass.length() < 8) {
        error = "admin.password must be at least 8 characters";
        return false;
    }

    JsonObjectConst power = doc["power"].as<JsonObjectConst>();
    String powerMode = power["mode"] | "normal";
    int sleepIntervalSec = power["sleep_interval_sec"] | 300;
    int webWindowSec = power["web_window_sec"] | 60;
    int wifiTimeoutMs = power["wifi_timeout_ms"] | 20000;
    int wifiRetrySec = power["wifi_retry_interval_sec"] | 30;

    if (powerMode != "normal" && powerMode != "battery") {
        error = "power.mode must be 'normal' or 'battery'";
        return false;
    }
    if (sleepIntervalSec < 30 || sleepIntervalSec > 86400) {
        error = "power.sleep_interval_sec must be between 30 and 86400";
        return false;
    }
    if (webWindowSec < 10 || webWindowSec > sleepIntervalSec) {
        error = "power.web_window_sec must be between 10 and sleep_interval_sec";
        return false;
    }
    if (wifiTimeoutMs < 5000 || wifiTimeoutMs > 60000) {
        error = "power.wifi_timeout_ms must be between 5000 and 60000";
        return false;
    }
    if (wifiRetrySec < 10 || wifiRetrySec > 3600) {
        error = "power.wifi_retry_interval_sec must be between 10 and 3600";
        return false;
    }

    return true;
}

inline JsonDocument loadConfig() {
    JsonDocument doc;

    if (!SPIFFS.exists(CONFIG_FILE)) {
        DBG_INFOLN("[Config] No config file, using defaults");
        return doc;
    }

    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        DBG_ERRORLN("[Config] Failed to open config file");
        return doc;
    }

    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        DBG_ERROR("[Config] Parse error: %s\n", err.c_str());
        return JsonDocument();
    }

    applyConfigDefaults(doc);
    DBG_INFOLN("[Config] Loaded OK");
    return doc;
}

inline bool saveConfig(const JsonDocument& doc) {
    if (SPIFFS.exists(CONFIG_TMP_FILE)) {
        SPIFFS.remove(CONFIG_TMP_FILE);
    }

    File file = SPIFFS.open(CONFIG_TMP_FILE, "w");
    if (!file) {
        DBG_ERRORLN("[Config] Failed to write temp config file");
        return false;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        file.close();
        SPIFFS.remove(CONFIG_TMP_FILE);
        DBG_ERRORLN("[Config] Failed to serialize config");
        return false;
    }

    file.flush();
    file.close();

    if (SPIFFS.exists(CONFIG_FILE) && !SPIFFS.remove(CONFIG_FILE)) {
        SPIFFS.remove(CONFIG_TMP_FILE);
        DBG_ERRORLN("[Config] Failed to replace old config");
        return false;
    }

    if (!SPIFFS.rename(CONFIG_TMP_FILE, CONFIG_FILE)) {
        SPIFFS.remove(CONFIG_TMP_FILE);
        DBG_ERRORLN("[Config] Failed to finalize config save");
        return false;
    }

    DBG_INFOLN("[Config] Saved OK");
    return true;
}

inline void createDefaultConfig() {
    if (SPIFFS.exists(CONFIG_FILE)) return;

    DBG_INFOLN("[Config] Creating default config...");

    JsonDocument doc;
    applyConfigDefaults(doc);
    saveConfig(doc);
}

#endif // CONFIG_MANAGER_H
