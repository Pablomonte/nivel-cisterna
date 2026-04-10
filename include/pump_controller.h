#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"

/**
 * Pump controller with hysteresis and safety timeout.
 * 
 * Behavior:
 *   - Turns ON  when level drops below (lowThreshold - hysteresis)
 *   - Turns OFF when level rises above (highThreshold + hysteresis)
 *   - Emergency shutoff if pump runs longer than timeout
 *   - Manual override from web interface
 * 
 *      OFF zone          HYSTERESIS          ON zone
 *   ◄──────────────┤  low_threshold  ├──────────────►
 */
enum PumpState {
    PUMP_OFF,
    PUMP_ON,
    PUMP_MANUAL_ON,
    PUMP_MANUAL_OFF,
    PUMP_TIMEOUT    // Safety shutoff
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
    unsigned long totalRuntimeMs;  // Accumulated pump runtime

    void setRelay(bool on) {
        bool pinState = activeLow ? !on : on;
        digitalWrite(relayPin, pinState);
    }

public:
    PumpController() : enabled(false), relayPin(26), activeLow(false),
                        autoMode(true), lowThreshold(20), highThreshold(90),
                        hysteresis(5), timeoutMs(30UL * 60 * 1000),
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

        DBG_INFO("[Pump] %s pin=%d low=%.0f high=%.0f hyst=%.0f\n",
                 enabled ? "enabled" : "disabled",
                 relayPin, lowThreshold, highThreshold, hysteresis);
    }

    bool init() {
        if (!enabled) return false;
        pinMode(relayPin, OUTPUT);
        setRelay(false);
        state = PUMP_OFF;
        DBG_INFO("[Pump] Init OK, relay pin %d\n", relayPin);
        return true;
    }

    /**
     * Update pump state based on current level.
     * Call this after each sensor reading.
     */
    void update(float levelPercent) {
        if (!enabled || !autoMode) return;

        // Safety timeout check
        if (state == PUMP_ON && pumpStartTime > 0) {
            if (millis() - pumpStartTime > timeoutMs) {
                DBG_ERROR("[Pump] TIMEOUT! Shutting off after %lu min\n",
                          timeoutMs / 60000);
                totalRuntimeMs += millis() - pumpStartTime;
                setRelay(false);
                state = PUMP_TIMEOUT;
                return;
            }
        }

        switch (state) {
            case PUMP_OFF:
                if (levelPercent < (lowThreshold - hysteresis)) {
                    DBG_INFO("[Pump] ON (level=%.1f < %.1f)\n",
                             levelPercent, lowThreshold - hysteresis);
                    setRelay(true);
                    pumpStartTime = millis();
                    state = PUMP_ON;
                }
                break;

            case PUMP_ON:
                if (levelPercent > (highThreshold + hysteresis)) {
                    DBG_INFO("[Pump] OFF (level=%.1f > %.1f)\n",
                             levelPercent, highThreshold + hysteresis);
                    totalRuntimeMs += millis() - pumpStartTime;
                    setRelay(false);
                    state = PUMP_OFF;
                }
                break;

            case PUMP_TIMEOUT:
                // Stay in timeout until manual reset
                break;

            case PUMP_MANUAL_ON:
            case PUMP_MANUAL_OFF:
                // Manual override - don't change automatically
                break;
        }
    }

    // Manual controls (from web interface)
    void manualOn() {
        if (!enabled) return;
        setRelay(true);
        pumpStartTime = millis();
        state = PUMP_MANUAL_ON;
        DBG_INFO("[Pump] Manual ON\n");
    }

    void manualOff() {
        if (!enabled) return;
        if (pumpStartTime > 0) totalRuntimeMs += millis() - pumpStartTime;
        setRelay(false);
        state = PUMP_MANUAL_OFF;
        DBG_INFO("[Pump] Manual OFF\n");
    }

    void resetToAuto() {
        if (!enabled) return;
        setRelay(false);
        state = PUMP_OFF;
        autoMode = true;
        DBG_INFO("[Pump] Reset to AUTO\n");
    }

    // Status
    bool isOn() const { return state == PUMP_ON || state == PUMP_MANUAL_ON; }
    bool isEnabled() const { return enabled; }
    PumpState getState() const { return state; }
    unsigned long getTotalRuntimeSec() const { return totalRuntimeMs / 1000; }

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

    String getGrafanaFields() {
        char buf[64];
        snprintf(buf, sizeof(buf), "pump_state=%d,pump_runtime=%lu",
                 isOn() ? 1 : 0, getTotalRuntimeSec());
        return String(buf);
    }
};

#endif // PUMP_CONTROLLER_H
