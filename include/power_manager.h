#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "esp_sleep.h"
#include "debug.h"

#define RTC_MAGIC 0xC157E2AUL

struct RtcState {
    uint32_t magic;
    uint32_t bootCount;
    float    lastLevel;
    bool     telegramLowLevelSent;
    bool     telegramSensorFailSent;
    time_t   lastNtpSync;
};

RTC_DATA_ATTR RtcState rtcState;

class PowerManager {
private:
    String mode;
    unsigned long sleepIntervalSec;
    unsigned long webWindowMs;
    unsigned long wifiTimeoutMs;
    unsigned long wifiRetryIntervalMs;
    unsigned long webWindowStartMs;
    bool firstBoot;

public:
    PowerManager() : mode("normal"), sleepIntervalSec(300), webWindowMs(60000),
                     wifiTimeoutMs(20000), wifiRetryIntervalMs(120000),
                     webWindowStartMs(0), firstBoot(false) {}

    void loadFromConfig(JsonObject cfg) {
        mode = cfg["mode"] | "normal";
        sleepIntervalSec = cfg["sleep_interval_sec"] | 300;
        webWindowMs = (unsigned long)(cfg["web_window_sec"] | 60) * 1000UL;
        wifiTimeoutMs = cfg["wifi_timeout_ms"] | 20000;
        wifiRetryIntervalMs = (unsigned long)(cfg["wifi_retry_interval_sec"] | 120) * 1000UL;

        if (rtcState.magic != RTC_MAGIC) {
            memset(&rtcState, 0, sizeof(rtcState));
            rtcState.magic = RTC_MAGIC;
            rtcState.lastLevel = -1.0f;
            firstBoot = true;
        }
        rtcState.bootCount++;

        DBG_INFO("[Power] mode=%s boot=%lu%s\n",
                 mode.c_str(), rtcState.bootCount, firstBoot ? " (first)" : "");
    }

    bool isBatteryMode() const { return mode == "battery"; }
    bool isFirstBoot() const { return firstBoot; }
    uint32_t getBootCount() const { return rtcState.bootCount; }
    unsigned long getWifiRetryIntervalMs() const { return wifiRetryIntervalMs; }

    bool connectWithTimeout(const String& ssid, const String& pass) {
        if (ssid.length() == 0) return false;

        WiFi.begin(ssid.c_str(), pass.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start >= wifiTimeoutMs) {
                DBG_INFOLN("[Power] WiFi connect timeout");
                return false;
            }
            delay(100);
        }
        DBG_INFO("[Power] WiFi connected. IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    void startWebWindow() {
        webWindowStartMs = millis();
        DBG_INFO("[Power] Web window open for %lu s\n", webWindowMs / 1000UL);
    }

    bool isWebWindowActive() const {
        return millis() - webWindowStartMs < webWindowMs;
    }

    void enterDeepSleep() {
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        DBG_INFO("[Power] Deep sleep %lu s (boot=%lu)\n",
                 sleepIntervalSec, rtcState.bootCount);
        delay(50);

        esp_sleep_enable_timer_wakeup(sleepIntervalSec * 1000000ULL);
        esp_deep_sleep_start();
    }
};

#endif // POWER_MANAGER_H
