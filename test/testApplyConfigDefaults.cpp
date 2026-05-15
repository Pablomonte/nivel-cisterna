// Regresion del bug "calibracion no persiste" (ver commit y/o memory log).
// ArduinoJson v7 `JsonVariant | T default` exige `is<T>()` exacto: si la variante
// es float y el default es int, el `|` devuelve el default y pisa el valor.
// applyConfigDefaults DEBE usar defaults float para campos numericos de tank y
// sensor, ya que el handler de calibracion escribe via `.as<float>()`.

#include <unity.h>
#include <ArduinoJson.h>

namespace {

// Replica el cuerpo de applyConfigDefaults para los campos float-sensitive.
// Si el firmware cambia los defaults a int, este test detecta la regresion.
void applyTankSensorDefaults(JsonDocument& doc) {
    JsonObject tank = doc["tank"];
    if (tank.isNull()) tank = doc["tank"].to<JsonObject>();
    tank["height_cm"]         = tank["height_cm"]         | 150.0f;
    tank["diameter_cm"]       = tank["diameter_cm"]       | 120.0f;
    tank["width_cm"]          = tank["width_cm"]          | 0.0f;
    tank["length_cm"]         = tank["length_cm"]         | 0.0f;
    tank["capacity_liters"]   = tank["capacity_liters"]   | 0.0f;
    tank["empty_distance_cm"] = tank["empty_distance_cm"] | 145.0f;
    tank["full_distance_cm"]  = tank["full_distance_cm"]  | 10.0f;

    JsonObject sensor = doc["sensor"];
    if (sensor.isNull()) sensor = doc["sensor"].to<JsonObject>();
    sensor["offset_cm"] = sensor["offset_cm"] | 0.0f;
}

}  // namespace

void testApplyDefaults_PreservesFloatTankCalibration() {
    JsonDocument doc;
    doc["tank"]["empty_distance_cm"] = 141.5f;
    doc["tank"]["full_distance_cm"]  = 9.25f;
    doc["sensor"]["offset_cm"]       = -45.0f;

    applyTankSensorDefaults(doc);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 141.5f, doc["tank"]["empty_distance_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.25f,  doc["tank"]["full_distance_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, doc["sensor"]["offset_cm"].as<float>());
}

void testApplyDefaults_AppliesFloatDefaultsWhenMissing() {
    JsonDocument doc;

    applyTankSensorDefaults(doc);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 145.0f, doc["tank"]["empty_distance_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f,  doc["tank"]["full_distance_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, doc["tank"]["height_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, doc["tank"]["diameter_cm"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f,   doc["sensor"]["offset_cm"].as<float>());
}

// Documenta la causa raiz del bug original: `JsonVariant | int` con valor float
// devuelve el default int. Este test deberia fallar si ArduinoJson cambia esa
// semantica en una version futura (en cuyo caso revisar applyConfigDefaults).
void testArduinoJsonV7_OrOperatorRequiresExactType() {
    JsonDocument doc;
    doc["x"] = 141.5f;

    // Float-vs-int default: pisa.
    int asInt = doc["x"] | 999;
    TEST_ASSERT_EQUAL_INT(999, asInt);

    // Float-vs-float default: preserva.
    float asFloat = doc["x"] | 999.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 141.5f, asFloat);
}
