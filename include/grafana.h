#ifndef GRAFANA_H
#define GRAFANA_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "debug.h"

class GrafanaReporter {
private:
    String url;
    String token;
    String deviceName;
    unsigned long sendIntervalMs;
    unsigned long timeoutMs;
    unsigned long lastAttemptTime;
    unsigned long lastSuccessTime;
    int lastHttpCode;
    bool configured;

    String escapeTagValue(const String& value) const {
        String escaped;
        escaped.reserve(value.length() + 8);

        for (size_t i = 0; i < value.length(); i++) {
            char c = value[i];
            if (c == ' ' || c == ',' || c == '=') {
                escaped += '\\';
            }
            escaped += c;
        }

        return escaped;
    }

public:
    GrafanaReporter() : sendIntervalMs(10000), timeoutMs(5000), lastAttemptTime(0),
                        lastSuccessTime(0), lastHttpCode(0), configured(false) {}

    void loadFromConfig(JsonObject grafanaCfg, const char* devName) {
        url = grafanaCfg["url"] | "";
        token = grafanaCfg["token"] | "";
        sendIntervalMs = (grafanaCfg["send_interval_sec"] | 10) * 1000UL;
        timeoutMs = grafanaCfg["timeout_ms"] | 5000UL;
        deviceName = String(devName);
        configured = url.length() > 0;

        if (configured) {
            DBG_INFO("[Grafana] URL=%s interval=%lus timeout=%lums\n",
                     url.c_str(), sendIntervalMs / 1000UL, timeoutMs);
        } else {
            DBG_INFOLN("[Grafana] Not configured (no URL)");
        }
    }

    bool isConfigured() const { return configured; }
    int getLastHttpCode() const { return lastHttpCode; }
    unsigned long getLastSuccessAgeSec() const {
        if (lastSuccessTime == 0) return millis() / 1000UL;
        return (millis() - lastSuccessTime) / 1000UL;
    }

    bool shouldSend() const {
        if (!configured || WiFi.status() != WL_CONNECTED) return false;
        return millis() - lastAttemptTime >= sendIntervalMs;
    }

    bool send(const String& fields) {
        if (!configured || WiFi.status() != WL_CONNECTED) return false;

        lastAttemptTime = millis();

        String payload;
        payload.reserve(fields.length() + deviceName.length() + 32);
        payload = "cisterna,device=";
        payload += escapeTagValue(deviceName);
        payload += " ";
        payload += fields;

        DBG_VERBOSE("[Grafana] POST: %s\n", payload.c_str());

        HTTPClient http;
        http.setConnectTimeout(timeoutMs);
        http.setTimeout(timeoutMs);

        if (!http.begin(url)) {
            lastHttpCode = -1;
            DBG_ERRORLN("[Grafana] Failed to open HTTP session");
            return false;
        }

        http.addHeader("Content-Type", "text/plain");
        if (token.length() > 0) {
            http.addHeader("Authorization", token);
        }

        lastHttpCode = http.POST(payload);
        http.end();

        if (lastHttpCode >= 200 && lastHttpCode < 300) {
            lastSuccessTime = lastAttemptTime;
            DBG_VERBOSE("[Grafana] OK (%d)\n", lastHttpCode);
            return true;
        }

        DBG_ERROR("[Grafana] Error: %d\n", lastHttpCode);
        return false;
    }
};

#endif // GRAFANA_H
