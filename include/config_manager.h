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

    JsonObject tank = doc["tank"];
    if (tank.isNull()) tank = doc["tank"].to<JsonObject>();
    tank["shape"] = tank["shape"] | "cylindrical";
    tank["height_cm"] = tank["height_cm"] | 150;
    tank["diameter_cm"] = tank["diameter_cm"] | 120;
    tank["width_cm"] = tank["width_cm"] | 0;
    tank["length_cm"] = tank["length_cm"] | 0;
    tank["capacity_liters"] = tank["capacity_liters"] | 0;
    tank["empty_distance_cm"] = tank["empty_distance_cm"] | 145;
    tank["full_distance_cm"] = tank["full_distance_cm"] | 10;

    JsonObject grafana = doc["grafana"];
    if (grafana.isNull()) grafana = doc["grafana"].to<JsonObject>();
    grafana["url"] = grafana["url"] | "";
    grafana["token"] = grafana["token"] | "";
    grafana["send_interval_sec"] = grafana["send_interval_sec"] | 10;
    grafana["timeout_ms"] = grafana["timeout_ms"] | 5000;

    JsonObject pump = doc["pump"];
    if (pump.isNull()) pump = doc["pump"].to<JsonObject>();
    pump["enabled"] = pump["enabled"] | false;
    pump["relay_pin"] = pump["relay_pin"] | 26;
    pump["active_low"] = pump["active_low"] | false;
    pump["auto_mode"] = pump["auto_mode"] | true;
    pump["low_threshold"] = pump["low_threshold"] | 20.0f;
    pump["high_threshold"] = pump["high_threshold"] | 90.0f;
    pump["hysteresis"] = pump["hysteresis"] | 5.0f;
    pump["timeout_min"] = pump["timeout_min"] | 30;

    JsonObject admin = doc["admin"];
    if (admin.isNull()) admin = doc["admin"].to<JsonObject>();
    admin["username"] = admin["username"] | "admin";
    admin["password"] = admin["password"] | "";

    JsonObject telegram = doc["telegram"];
    if (telegram.isNull()) telegram = doc["telegram"].to<JsonObject>();
    telegram["enabled"] = telegram["enabled"] | false;
    telegram["chat_id"] = telegram["chat_id"] | "";
    telegram["bot_token"] = telegram["bot_token"] | "";
    telegram["low_level_threshold_pct"] = telegram["low_level_threshold_pct"] | 15;
    telegram["recovery_threshold_pct"] = telegram["recovery_threshold_pct"] | 30;
    telegram["sensor_failure_threshold"] = telegram["sensor_failure_threshold"] | 3;
    telegram["cooldown_sec"] = telegram["cooldown_sec"] | 900;
}

inline bool validateConfig(const JsonDocument& doc, String& error) {
    String deviceName = doc["device_name"] | "";
    if (deviceName.length() == 0) {
        error = "device_name is required";
        return false;
    }

    JsonObjectConst sensor = doc["sensor"].as<JsonObjectConst>();
    int samples = sensor["samples"] | 5;
    int readIntervalSec = sensor["read_interval_sec"] | 10;
    if (samples < 1 || samples > 15) {
        error = "sensor.samples must be between 1 and 15";
        return false;
    }
    if (readIntervalSec < 1) {
        error = "sensor.read_interval_sec must be at least 1";
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

    JsonObjectConst pump = doc["pump"].as<JsonObjectConst>();
    float lowThreshold = pump["low_threshold"] | 20.0f;
    float highThreshold = pump["high_threshold"] | 90.0f;
    float hysteresis = pump["hysteresis"] | 5.0f;
    int timeoutMin = pump["timeout_min"] | 30;
    if (lowThreshold < 0 || lowThreshold > 100 ||
        highThreshold < 0 || highThreshold > 100 ||
        lowThreshold >= highThreshold) {
        error = "pump thresholds must satisfy 0 <= low < high <= 100";
        return false;
    }
    if (hysteresis < 0 || hysteresis > 20) {
        error = "pump.hysteresis must be between 0 and 20";
        return false;
    }
    if (timeoutMin < 1) {
        error = "pump.timeout_min must be at least 1";
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

    JsonObjectConst telegram = doc["telegram"].as<JsonObjectConst>();
    bool telegramEnabled = telegram["enabled"] | false;
    int lowLevelThreshold = telegram["low_level_threshold_pct"] | 15;
    int recoveryThreshold = telegram["recovery_threshold_pct"] | 30;
    int sensorFailureThreshold = telegram["sensor_failure_threshold"] | 3;
    int cooldownSec = telegram["cooldown_sec"] | 900;

    if (telegramEnabled) {
        String chatId = telegram["chat_id"] | "";
        String botToken = telegram["bot_token"] | "";
        if (chatId.length() == 0) {
            error = "telegram.chat_id is required when telegram is enabled";
            return false;
        }
        if (botToken.length() == 0) {
            error = "telegram.bot_token is required when telegram is enabled";
            return false;
        }
    }
    if (lowLevelThreshold < 1 || lowLevelThreshold > 99) {
        error = "telegram.low_level_threshold_pct must be between 1 and 99";
        return false;
    }
    if (recoveryThreshold <= lowLevelThreshold || recoveryThreshold > 100) {
        error = "telegram.recovery_threshold_pct must be greater than low level threshold and <= 100";
        return false;
    }
    if (sensorFailureThreshold < 1) {
        error = "telegram.sensor_failure_threshold must be at least 1";
        return false;
    }
    if (cooldownSec < 60) {
        error = "telegram.cooldown_sec must be at least 60";
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
