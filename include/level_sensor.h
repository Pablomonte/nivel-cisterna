#ifndef LEVEL_SENSOR_H
#define LEVEL_SENSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"
#include "tank.h"

// HC-SR04 / JSN-SR04T constants
#define SPEED_OF_SOUND_CM_US  0.0343f  // cm/µs at ~20°C
#define MAX_DISTANCE_CM       400.0f
#define MIN_DISTANCE_CM       2.0f
#define PULSE_TIMEOUT_US      30000    // ~5m max

/**
 * Ultrasonic distance sensor for water level measurement.
 * 
 * Supports HC-SR04 (air) and JSN-SR04T (waterproof).
 * Uses median filtering to reject outlier readings.
 */
class LevelSensor {
private:
    int triggerPin;
    int echoPin;
    int numSamples;
    unsigned long readIntervalMs;
    unsigned long lastReadTime;

    float lastDistance_cm;
    float lastLevel;
    float lastVolume;
    bool sensorOk;

    Tank* tank;

    /**
     * Single distance measurement in cm.
     * Returns -1 on timeout/error.
     */
    float measureOnce() {
        // Trigger pulse
        digitalWrite(triggerPin, LOW);
        delayMicroseconds(2);
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(triggerPin, LOW);

        // Measure echo duration
        unsigned long duration = pulseIn(echoPin, HIGH, PULSE_TIMEOUT_US);

        if (duration == 0) {
            return -1;  // Timeout - no echo
        }

        float distance = (duration * SPEED_OF_SOUND_CM_US) / 2.0f;

        if (distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM) {
            return -1;  // Out of range
        }

        return distance;
    }

    /**
     * Median filter: take N samples, return the median.
     * Rejects -1 (error) readings.
     */
    float measureMedian() {
        float readings[numSamples];
        int validCount = 0;

        for (int i = 0; i < numSamples; i++) {
            float d = measureOnce();
            if (d > 0) {
                readings[validCount++] = d;
            }
            delay(30);  // Wait between pings (sensor recovery)
        }

        if (validCount == 0) {
            DBG_ERROR("[Sensor] No valid readings!\n");
            return -1;
        }

        // Sort for median
        for (int i = 0; i < validCount - 1; i++) {
            for (int j = i + 1; j < validCount; j++) {
                if (readings[j] < readings[i]) {
                    float tmp = readings[i];
                    readings[i] = readings[j];
                    readings[j] = tmp;
                }
            }
        }

        float median = readings[validCount / 2];
        DBG_VERBOSE("[Sensor] %d/%d valid, median=%.1f cm\n",
                    validCount, numSamples, median);
        return median;
    }

public:
    LevelSensor() : triggerPin(5), echoPin(18), numSamples(5),
                     readIntervalMs(10000), lastReadTime(0),
                     lastDistance_cm(-1), lastLevel(-1), lastVolume(-1),
                     sensorOk(false), tank(nullptr) {}

    void setTank(Tank* t) { tank = t; }

    void loadFromConfig(JsonObject cfg) {
        triggerPin = cfg["trigger_pin"] | 5;
        echoPin = cfg["echo_pin"] | 18;
        numSamples = cfg["samples"] | 5;
        readIntervalMs = (cfg["read_interval_sec"] | 10) * 1000UL;

        if (numSamples < 1) numSamples = 1;
        if (numSamples > 15) numSamples = 15;

        DBG_INFO("[Sensor] trig=%d echo=%d samples=%d interval=%lums\n",
                 triggerPin, echoPin, numSamples, readIntervalMs);
    }

    bool init() {
        pinMode(triggerPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(triggerPin, LOW);

        // Test read
        delay(100);
        float testDist = measureOnce();
        sensorOk = (testDist > 0);

        if (sensorOk) {
            DBG_INFO("[Sensor] OK, test=%.1f cm\n", testDist);
        } else {
            DBG_ERROR("[Sensor] Init failed - no echo\n");
        }

        return sensorOk;
    }

    /**
     * Read sensor if interval has elapsed.
     * Returns true if new data is available.
     */
    bool update() {
        unsigned long now = millis();
        if (now - lastReadTime < readIntervalMs) {
            return false;
        }
        lastReadTime = now;

        float distance = measureMedian();

        if (distance < 0) {
            sensorOk = false;
            return false;
        }

        sensorOk = true;
        lastDistance_cm = distance;

        if (tank) {
            lastLevel = tank->distanceToLevel(distance);
            lastVolume = tank->levelToVolume(lastLevel);
        }

        DBG_INFO("[Nivel] dist=%.1f cm  nivel=%.1f%%  vol=%.0f L\n",
                 lastDistance_cm, lastLevel, lastVolume);

        return true;
    }

    // Getters
    float getDistance() const { return lastDistance_cm; }
    float getLevel() const { return lastLevel; }
    float getVolume() const { return lastVolume; }
    bool isOk() const { return sensorOk; }
    unsigned long getReadInterval() const { return readIntervalMs; }

    /**
     * Format measurements as InfluxDB line protocol fields.
     * Example: "level=85.2,distance=23.4,volume=1200.5"
     */
    String getMeasurementsString() {
        char buf[80];
        snprintf(buf, sizeof(buf), "level=%.1f,distance=%.1f,volume=%.0f",
                 lastLevel, lastDistance_cm, lastVolume);
        return String(buf);
    }
};

#endif // LEVEL_SENSOR_H
