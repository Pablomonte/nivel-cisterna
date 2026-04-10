#ifndef TANK_H
#define TANK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "debug.h"

enum TankShape {
    TANK_CYLINDRICAL,
    TANK_RECTANGULAR
};

/**
 * Tank geometry and level calculations.
 * 
 * The ultrasonic sensor is mounted at the top of the tank, pointing down.
 * It measures the distance to the water surface.
 * 
 *   Sensor ──┐
 *            │  ← empty_distance_cm (sensor to bottom when empty)
 *            │
 *   Water ───┤  ← distance reading
 *   ~~~~~~~~ │
 *   ~~~~~~~~ │  ← water column height = empty_distance - distance
 *   ~~~~~~~~ │
 *   Bottom ──┘
 * 
 * Level % = (water_height / tank_height) * 100
 */
class Tank {
private:
    TankShape shape;
    float height_cm;
    float diameter_cm;
    float width_cm;
    float length_cm;
    float capacityOverride_liters;  // 0 = auto-calculate

    // Sensor calibration
    float emptyDistance_cm;   // Distance reading when tank is empty
    float fullDistance_cm;    // Distance reading when tank is full

public:
    Tank() : shape(TANK_CYLINDRICAL), height_cm(150), diameter_cm(120),
             width_cm(0), length_cm(0), capacityOverride_liters(0),
             emptyDistance_cm(145), fullDistance_cm(10) {}

    void loadFromConfig(JsonObject cfg) {
        const char* shapeStr = cfg["shape"] | "cylindrical";
        if (strcmp(shapeStr, "rectangular") == 0) {
            shape = TANK_RECTANGULAR;
        } else {
            shape = TANK_CYLINDRICAL;
        }

        height_cm = cfg["height_cm"] | 150.0f;
        diameter_cm = cfg["diameter_cm"] | 120.0f;
        width_cm = cfg["width_cm"] | 0.0f;
        length_cm = cfg["length_cm"] | 0.0f;
        capacityOverride_liters = cfg["capacity_liters"] | 0.0f;
        emptyDistance_cm = cfg["empty_distance_cm"] | 145.0f;
        fullDistance_cm = cfg["full_distance_cm"] | 10.0f;

        DBG_INFO("[Tank] %s h=%.0f empty=%.0f full=%.0f\n",
                 shapeStr, height_cm, emptyDistance_cm, fullDistance_cm);
    }

    /**
     * Convert raw sensor distance to water level percentage.
     * Returns 0-100 (clamped).
     */
    float distanceToLevel(float distance_cm) {
        if (emptyDistance_cm <= fullDistance_cm) {
            DBG_ERROR("[Tank] Bad calibration: empty <= full\n");
            return -1;
        }

        float level = (emptyDistance_cm - distance_cm) /
                       (emptyDistance_cm - fullDistance_cm) * 100.0f;
        return constrain(level, 0.0f, 100.0f);
    }

    /**
     * Calculate volume in liters from level percentage.
     */
    float levelToVolume(float levelPercent) {
        if (capacityOverride_liters > 0) {
            return capacityOverride_liters * (levelPercent / 100.0f);
        }

        float waterHeight_cm = height_cm * (levelPercent / 100.0f);
        float volume_cm3 = 0;

        if (shape == TANK_CYLINDRICAL) {
            float radius = diameter_cm / 2.0f;
            volume_cm3 = PI * radius * radius * waterHeight_cm;
        } else {
            volume_cm3 = width_cm * length_cm * waterHeight_cm;
        }

        return volume_cm3 / 1000.0f;  // cm³ → liters
    }

    /**
     * Get total tank capacity in liters.
     */
    float getCapacity() {
        if (capacityOverride_liters > 0) return capacityOverride_liters;
        return levelToVolume(100.0f);
    }

    float getEmptyDistance() const { return emptyDistance_cm; }
    float getFullDistance() const { return fullDistance_cm; }
    float getHeight() const { return height_cm; }
    TankShape getShape() const { return shape; }
};

#endif // TANK_H
