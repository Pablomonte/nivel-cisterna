#ifndef GRAFANA_H
#define GRAFANA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "debug.h"

/**
 * Send measurements to Grafana/InfluxDB via HTTP POST.
 * Uses InfluxDB v2 line protocol.
 * 
 * Format: measurement,tag=value field1=val1,field2=val2
 * Example: cisterna,device=cisterna-01 level=85.2,distance=23.4,volume=1200
 */
class GrafanaReporter {
private:
    String url;
    String token;
    String deviceName;
    unsigned long sendIntervalMs;
    unsigned long lastSendTime;
    bool configured;

public:
    GrafanaReporter() : sendIntervalMs(10000), lastSendTime(0), configured(false) {}

    void loadFromConfig(JsonObject grafanaCfg, const char* devName) {
        url = grafanaCfg["url"] | "";
        token = grafanaCfg["token"] | "";
        sendIntervalMs = (grafanaCfg["send_interval_sec"] | 10) * 1000UL;
        deviceName = String(devName);

        configured = (url.length() > 0);

        if (configured) {
            DBG_INFO("[Grafana] URL=%s interval=%lus\n",
                     url.c_str(), sendIntervalMs / 1000);
        } else {
            DBG_INFOLN("[Grafana] Not configured (no URL)");
        }
    }

    bool isConfigured() const { return configured; }

    bool shouldSend() {
        if (!configured) return false;
        if (WiFi.status() != WL_CONNECTED) return false;
        return (millis() - lastSendTime >= sendIntervalMs);
    }

    /**
     * Send data to Grafana.
     * @param fields InfluxDB fields string, e.g. "level=85.2,distance=23.4"
     * @return true if send was successful
     */
    bool send(const String& fields) {
        if (!configured || WiFi.status() != WL_CONNECTED) return false;

        lastSendTime = millis();

        // Build line protocol: cisterna,device=name fields
        String payload = "cisterna,device=" + deviceName + " " + fields;

        DBG_VERBOSE("[Grafana] POST: %s\n", payload.c_str());

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "text/plain");

        if (token.length() > 0) {
            http.addHeader("Authorization", "Bearer " + token);
        }

        int httpCode = http.POST(payload);
        http.end();

        if (httpCode >= 200 && httpCode < 300) {
            DBG_VERBOSE("[Grafana] OK (%d)\n", httpCode);
            return true;
        } else {
            DBG_ERROR("[Grafana] Error: %d\n", httpCode);
            return false;
        }
    }
};

#endif // GRAFANA_H
