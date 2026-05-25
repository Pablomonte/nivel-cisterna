#ifndef TANK_H
#define TANK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"

enum TankShape {
    TANK_CYLINDRICAL,
    TANK_RECTANGULAR
};

class Tank {
private:
    TankShape shape;
    float height_cm;
    float diameter_cm;
    float width_cm;
    float length_cm;
    float capacityOverride_liters;
    float emptyDistance_cm;
    float fullDistance_cm;

    void setSafeDefaults() {
        shape = TANK_CYLINDRICAL;
        height_cm = 150.0f;
        diameter_cm = 120.0f;
        width_cm = 0.0f;
        length_cm = 0.0f;
        capacityOverride_liters = 0.0f;
        // 0/0 = sin calibrar (distanceToLevel devolvera -1 hasta que el
        // usuario configure ambos valores desde la UI).
        emptyDistance_cm = 0.0f;
        fullDistance_cm = 0.0f;
    }

public:
    Tank() {
        setSafeDefaults();
    }

    void loadFromConfig(JsonObject cfg) {
        setSafeDefaults();

        const char* shapeStr = cfg["shape"] | "cylindrical";
        shape = strcmp(shapeStr, "rectangular") == 0 ? TANK_RECTANGULAR : TANK_CYLINDRICAL;

        height_cm = cfg["height_cm"] | height_cm;
        diameter_cm = cfg["diameter_cm"] | diameter_cm;
        width_cm = cfg["width_cm"] | width_cm;
        length_cm = cfg["length_cm"] | length_cm;
        capacityOverride_liters = cfg["capacity_liters"] | capacityOverride_liters;
        emptyDistance_cm = cfg["empty_distance_cm"] | emptyDistance_cm;
        fullDistance_cm = cfg["full_distance_cm"] | fullDistance_cm;

        if (height_cm <= 0) {
            DBG_ERRORLN("[Tank] Invalid height, using default 150 cm");
            height_cm = 150.0f;
        }

        if (capacityOverride_liters <= 0) {
            if (shape == TANK_CYLINDRICAL && diameter_cm <= 0) {
                DBG_ERRORLN("[Tank] Invalid diameter, using default 120 cm");
                diameter_cm = 120.0f;
            }
            if (shape == TANK_RECTANGULAR && (width_cm <= 0 || length_cm <= 0)) {
                DBG_ERRORLN("[Tank] Invalid rectangular dimensions, using defaults");
                width_cm = 100.0f;
                length_cm = 100.0f;
            }
        }

        bool uncalibrated = (emptyDistance_cm == 0.0f && fullDistance_cm == 0.0f);
        if (!uncalibrated && emptyDistance_cm <= fullDistance_cm) {
            DBG_ERRORLN("[Tank] Invalid calibration (empty <= full), marking uncalibrated");
            emptyDistance_cm = 0.0f;
            fullDistance_cm = 0.0f;
            uncalibrated = true;
        }

        DBG_INFO("[Tank] %s h=%.0f empty=%.0f full=%.0f%s\n",
                 shape == TANK_RECTANGULAR ? "rectangular" : "cylindrical",
                 height_cm, emptyDistance_cm, fullDistance_cm,
                 uncalibrated ? " (sin calibrar)" : "");
    }

    bool isCalibrated() const {
        return emptyDistance_cm > fullDistance_cm && emptyDistance_cm > 0.0f;
    }

    float distanceToLevel(float distance_cm) const {
        if (distance_cm < 0 || emptyDistance_cm <= fullDistance_cm) {
            return -1.0f;
        }

        float level = (emptyDistance_cm - distance_cm) /
                      (emptyDistance_cm - fullDistance_cm) * 100.0f;
        return constrain(level, 0.0f, 100.0f);
    }

    float levelToVolume(float levelPercent) const {
        if (levelPercent < 0.0f) {
            return -1.0f;
        }

        if (capacityOverride_liters > 0) {
            return capacityOverride_liters * (levelPercent / 100.0f);
        }

        float waterHeight_cm = height_cm * (levelPercent / 100.0f);
        float volume_cm3 = 0.0f;

        if (shape == TANK_CYLINDRICAL) {
            float radius = diameter_cm / 2.0f;
            volume_cm3 = PI * radius * radius * waterHeight_cm;
        } else {
            volume_cm3 = width_cm * length_cm * waterHeight_cm;
        }

        return volume_cm3 / 1000.0f;
    }

    float getCapacity() const {
        if (capacityOverride_liters > 0) return capacityOverride_liters;
        return levelToVolume(100.0f);
    }

    float getEmptyDistance() const { return emptyDistance_cm; }
    float getFullDistance() const { return fullDistance_cm; }
    float getHeight() const { return height_cm; }
    TankShape getShape() const { return shape; }
};

#endif // TANK_H
