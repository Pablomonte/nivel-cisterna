#ifndef LEVEL_SENSOR_H
#define LEVEL_SENSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"
#include "tank.h"

#define SPEED_OF_SOUND_CM_US  0.0343f
#define MAX_DISTANCE_CM       400.0f
#define MIN_DISTANCE_CM       2.0f
#define PULSE_TIMEOUT_US      30000
#define MAX_SENSOR_SAMPLES    15

class LevelSensor {
private:
    int triggerPin;
    int echoPin;
    int numSamples;
    unsigned long readIntervalMs;
    unsigned long lastReadTime;
    unsigned long lastSuccessfulReadTime;

    float lastDistance_cm;
    float lastLevel;
    float lastVolume;
    float lastSpread_cm;
    bool sensorOk;
    int consecutiveFailures;

    Tank* tank;

    float measureOnce() {
        digitalWrite(triggerPin, LOW);
        delayMicroseconds(2);
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(triggerPin, LOW);

        unsigned long duration = pulseIn(echoPin, HIGH, PULSE_TIMEOUT_US);
        if (duration == 0) {
            return -1.0f;
        }

        float distance = (duration * SPEED_OF_SOUND_CM_US) / 2.0f;
        if (distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM) {
            return -1.0f;
        }

        return distance;
    }

    float measureMedian() {
        float readings[MAX_SENSOR_SAMPLES];
        int validCount = 0;

        for (int i = 0; i < numSamples; i++) {
            float distance = measureOnce();
            if (distance > 0) {
                readings[validCount++] = distance;
            }
            delay(30);
        }

        if (validCount == 0) {
            lastSpread_cm = -1.0f;
            DBG_INFO("[Sensor] No echo (failures: %d)\n", consecutiveFailures + 1);
            return -1.0f;
        }

        for (int i = 0; i < validCount - 1; i++) {
            for (int j = i + 1; j < validCount; j++) {
                if (readings[j] < readings[i]) {
                    float tmp = readings[i];
                    readings[i] = readings[j];
                    readings[j] = tmp;
                }
            }
        }

        lastSpread_cm = readings[validCount - 1] - readings[0];
        float median = readings[validCount / 2];
        DBG_VERBOSE("[Sensor] %d/%d valid, median=%.1f cm spread=%.1f cm\n",
                    validCount, numSamples, median, lastSpread_cm);
        return median;
    }

public:
    LevelSensor() : triggerPin(5), echoPin(18), numSamples(5),
                    readIntervalMs(10000), lastReadTime(0), lastSuccessfulReadTime(0),
                    lastDistance_cm(-1.0f), lastLevel(-1.0f), lastVolume(-1.0f),
                    lastSpread_cm(-1.0f), sensorOk(false), consecutiveFailures(0),
                    tank(nullptr) {}

    void setTank(Tank* t) { tank = t; }

    void loadFromConfig(JsonObject cfg) {
        triggerPin = cfg["trigger_pin"] | 5;
        echoPin = cfg["echo_pin"] | 18;
        numSamples = cfg["samples"] | 5;
        readIntervalMs = (cfg["read_interval_sec"] | 10) * 1000UL;

        if (numSamples < 1) numSamples = 1;
        if (numSamples > MAX_SENSOR_SAMPLES) numSamples = MAX_SENSOR_SAMPLES;

        DBG_INFO("[Sensor] trig=%d echo=%d samples=%d interval=%lums\n",
                 triggerPin, echoPin, numSamples, readIntervalMs);
    }

    bool init() {
        pinMode(triggerPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(triggerPin, LOW);

        delay(100);
        float testDist = measureOnce();
        sensorOk = (testDist > 0);
        lastReadTime = millis() - readIntervalMs;

        if (sensorOk) {
            DBG_INFO("[Sensor] OK, test=%.1f cm\n", testDist);
        } else {
            DBG_INFO("[Sensor] No echo on init (sensor not connected?)\n");
        }

        return sensorOk;
    }

    bool update() {
        unsigned long now = millis();
        if (now - lastReadTime < readIntervalMs) {
            return false;
        }
        lastReadTime = now;

        float distance = measureMedian();
        if (distance < 0) {
            sensorOk = false;
            consecutiveFailures++;
            lastDistance_cm = -1.0f;
            lastLevel = -1.0f;
            lastVolume = -1.0f;
            return true;
        }

        sensorOk = true;
        consecutiveFailures = 0;
        lastSuccessfulReadTime = now;
        lastDistance_cm = distance;

        if (tank) {
            lastLevel = tank->distanceToLevel(distance);
            lastVolume = tank->levelToVolume(lastLevel);
        } else {
            lastLevel = -1.0f;
            lastVolume = -1.0f;
        }

        DBG_INFO("[Nivel] dist=%.1f cm nivel=%.1f%% vol=%.0f L\n",
                 lastDistance_cm, lastLevel, lastVolume);
        return true;
    }

    float getDistance() const { return lastDistance_cm; }
    float getLevel() const { return lastLevel; }
    float getVolume() const { return lastVolume; }
    float getSpreadCm() const { return lastSpread_cm; }
    bool isOk() const { return sensorOk; }
    int getConsecutiveFailures() const { return consecutiveFailures; }
    unsigned long getReadInterval() const { return readIntervalMs; }

    unsigned long getLastSuccessAgeSec() const {
        if (lastSuccessfulReadTime == 0) {
            return millis() / 1000UL;
        }
        return (millis() - lastSuccessfulReadTime) / 1000UL;
    }

    String getMeasurementsString() const {
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "level=%.1f,distance=%.1f,volume=%.0f,sensor_ok=%di,sensor_failures=%di,"
                 "sensor_spread_cm=%.1f,last_success_age_sec=%lui",
                 lastLevel, lastDistance_cm, lastVolume, sensorOk ? 1 : 0,
                 consecutiveFailures, lastSpread_cm, getLastSuccessAgeSec());
        return String(buf);
    }
};

#endif // LEVEL_SENSOR_H
