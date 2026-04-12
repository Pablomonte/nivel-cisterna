#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"

enum PumpState {
    PUMP_OFF = 0,
    PUMP_ON = 1,
    PUMP_MANUAL_ON = 2,
    PUMP_MANUAL_OFF = 3,
    PUMP_TIMEOUT = 4
};

class PumpController {
private:
    bool enabled;
    int relayPin;
    bool activeLow;
    bool autoMode;

    float lowThreshold;
    float highThreshold;
    float hysteresis;
    unsigned long timeoutMs;

    PumpState state;
    unsigned long pumpStartTime;
    unsigned long totalRuntimeMs;

    void setRelay(bool on) {
        bool pinState = activeLow ? !on : on;
        digitalWrite(relayPin, pinState);
    }

    void startPump(PumpState nextState) {
        if (!isOn()) {
            pumpStartTime = millis();
        }
        setRelay(true);
        state = nextState;
    }

    void stopPump(PumpState nextState) {
        if (isOn() && pumpStartTime > 0) {
            totalRuntimeMs += millis() - pumpStartTime;
        }
        pumpStartTime = 0;
        setRelay(false);
        state = nextState;
    }

public:
    PumpController() : enabled(false), relayPin(26), activeLow(false),
                       autoMode(true), lowThreshold(20.0f), highThreshold(90.0f),
                       hysteresis(5.0f), timeoutMs(30UL * 60UL * 1000UL),
                       state(PUMP_OFF), pumpStartTime(0), totalRuntimeMs(0) {}

    void loadFromConfig(JsonObject cfg) {
        enabled = cfg["enabled"] | false;
        relayPin = cfg["relay_pin"] | 26;
        activeLow = cfg["active_low"] | false;
        autoMode = cfg["auto_mode"] | true;
        lowThreshold = cfg["low_threshold"] | 20.0f;
        highThreshold = cfg["high_threshold"] | 90.0f;
        hysteresis = cfg["hysteresis"] | 5.0f;
        timeoutMs = (cfg["timeout_min"] | 30) * 60UL * 1000UL;

        DBG_INFO("[Pump] %s pin=%d low=%.0f high=%.0f hyst=%.0f auto=%s\n",
                 enabled ? "enabled" : "disabled",
                 relayPin, lowThreshold, highThreshold, hysteresis,
                 autoMode ? "yes" : "no");
    }

    bool init() {
        if (!enabled) return false;
        pinMode(relayPin, OUTPUT);
        setRelay(false);
        state = PUMP_OFF;
        pumpStartTime = 0;
        DBG_INFO("[Pump] Init OK, relay pin %d\n", relayPin);
        return true;
    }

    void update(float levelPercent) {
        if (!enabled) return;

        if (state == PUMP_ON && pumpStartTime > 0 && millis() - pumpStartTime > timeoutMs) {
            DBG_ERROR("[Pump] TIMEOUT after %lu min\n", timeoutMs / 60000UL);
            stopPump(PUMP_TIMEOUT);
            return;
        }

        if (!autoMode) return;
        if (levelPercent < 0.0f || levelPercent > 100.0f) return;

        switch (state) {
            case PUMP_OFF:
                if (levelPercent <= (lowThreshold - hysteresis)) {
                    DBG_INFO("[Pump] ON (level=%.1f <= %.1f)\n",
                             levelPercent, lowThreshold - hysteresis);
                    startPump(PUMP_ON);
                }
                break;

            case PUMP_ON:
                if (levelPercent >= (highThreshold + hysteresis)) {
                    DBG_INFO("[Pump] OFF (level=%.1f >= %.1f)\n",
                             levelPercent, highThreshold + hysteresis);
                    stopPump(PUMP_OFF);
                }
                break;

            case PUMP_TIMEOUT:
            case PUMP_MANUAL_ON:
            case PUMP_MANUAL_OFF:
                break;
        }
    }

    void manualOn() {
        if (!enabled) return;
        autoMode = false;
        startPump(PUMP_MANUAL_ON);
        DBG_INFOLN("[Pump] Manual ON");
    }

    void manualOff() {
        if (!enabled) return;
        autoMode = false;
        stopPump(PUMP_MANUAL_OFF);
        DBG_INFOLN("[Pump] Manual OFF");
    }

    void resetToAuto() {
        if (!enabled) return;
        stopPump(PUMP_OFF);
        autoMode = true;
        DBG_INFOLN("[Pump] Reset to AUTO");
    }

    bool isOn() const { return state == PUMP_ON || state == PUMP_MANUAL_ON; }
    bool isEnabled() const { return enabled; }
    bool isAutoMode() const { return autoMode; }
    PumpState getState() const { return state; }
    int getStateCode() const { return static_cast<int>(state); }

    unsigned long getTotalRuntimeSec() const {
        unsigned long runtimeMs = totalRuntimeMs;
        if (isOn() && pumpStartTime > 0) {
            runtimeMs += millis() - pumpStartTime;
        }
        return runtimeMs / 1000UL;
    }

    const char* getStateString() const {
        switch (state) {
            case PUMP_OFF:        return "off";
            case PUMP_ON:         return "on";
            case PUMP_MANUAL_ON:  return "manual_on";
            case PUMP_MANUAL_OFF: return "manual_off";
            case PUMP_TIMEOUT:    return "timeout";
            default:              return "unknown";
        }
    }

    String getGrafanaFields() const {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "pump_enabled=%di,pump_on=%di,pump_state_code=%di,pump_runtime_sec=%lui,pump_auto_mode=%di",
                 enabled ? 1 : 0, isOn() ? 1 : 0, getStateCode(),
                 getTotalRuntimeSec(), autoMode ? 1 : 0);
        return String(buf);
    }
};

#endif // PUMP_CONTROLLER_H
