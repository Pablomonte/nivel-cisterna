#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "debug.h"

#define CONFIG_FILE "/config.json"

/**
 * Load config from SPIFFS. Returns empty doc if file doesn't exist.
 */
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

    DBG_INFOLN("[Config] Loaded OK");
    return doc;
}

/**
 * Save config to SPIFFS.
 */
inline bool saveConfig(JsonDocument& doc) {
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        DBG_ERRORLN("[Config] Failed to write config file");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();
    DBG_INFOLN("[Config] Saved OK");
    return true;
}

/**
 * Create default config file if it doesn't exist.
 */
inline void createDefaultConfig() {
    if (SPIFFS.exists(CONFIG_FILE)) return;

    DBG_INFOLN("[Config] Creating default config...");

    JsonDocument doc;

    doc["device_name"] = "cisterna-01";

    JsonObject sensor = doc["sensor"].to<JsonObject>();
    sensor["trigger_pin"] = 5;
    sensor["echo_pin"] = 18;
    sensor["samples"] = 5;
    sensor["read_interval_sec"] = 10;

    JsonObject tank = doc["tank"].to<JsonObject>();
    tank["shape"] = "cylindrical";
    tank["height_cm"] = 150;
    tank["diameter_cm"] = 120;
    tank["capacity_liters"] = 0;
    tank["empty_distance_cm"] = 145;
    tank["full_distance_cm"] = 10;

    JsonObject grafana = doc["grafana"].to<JsonObject>();
    grafana["url"] = "";
    grafana["token"] = "";
    grafana["send_interval_sec"] = 10;

    saveConfig(doc);
}

#endif // CONFIG_MANAGER_H
