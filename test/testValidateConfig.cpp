// Reglas de validacion replicadas (con std::string) desde
// include/config_manager.h::validateConfig. Cualquier cambio en las reglas
// del firmware debe reflejarse aqui o el test fallara: bloqueo de regresiones.

#include <unity.h>
#include <string>

namespace {

bool validateDeviceName(const std::string& name, std::string& error) {
    if (name.empty()) {
        error = "device_name is required";
        return false;
    }
    if (name.size() > 31) {
        error = "device_name must be 31 characters or fewer";
        return false;
    }
    return true;
}

bool validateWifiSsid(const std::string& ssid, std::string& error) {
    if (ssid.size() > 32) {
        error = "wifi_ssid must be 32 characters or fewer";
        return false;
    }
    return true;
}

bool validateWifiPass(const std::string& pass, std::string& error) {
    if (!pass.empty() && (pass.size() < 8 || pass.size() > 63)) {
        error = "wifi_pass must be 8-63 characters (WPA2) or empty for AP-only";
        return false;
    }
    return true;
}

bool validateAdminPassword(const std::string& pass, std::string& error) {
    if (!pass.empty() && pass.size() < 8) {
        error = "admin.password must be at least 8 characters";
        return false;
    }
    return true;
}

bool validateSensorOffset(float offset, std::string& error) {
    if (offset < -50.0f || offset > 50.0f) {
        error = "sensor.offset_cm must be between -50 and 50";
        return false;
    }
    return true;
}

bool validateTankCalibration(float emptyCm, float fullCm, std::string& error) {
    if (emptyCm <= fullCm) {
        error = "tank.empty_distance_cm must be greater than tank.full_distance_cm";
        return false;
    }
    return true;
}

}  // namespace

void testValidateDeviceNameLength() {
    std::string err;

    TEST_ASSERT_FALSE(validateDeviceName("", err));
    TEST_ASSERT_EQUAL_STRING("device_name is required", err.c_str());

    err.clear();
    TEST_ASSERT_TRUE(validateDeviceName("cisterna-01", err));

    err.clear();
    TEST_ASSERT_TRUE(validateDeviceName(std::string(31, 'a'), err));

    err.clear();
    TEST_ASSERT_FALSE(validateDeviceName(std::string(32, 'a'), err));
    TEST_ASSERT_EQUAL_STRING("device_name must be 31 characters or fewer", err.c_str());
}

void testValidateWifiSsidLength() {
    std::string err;

    TEST_ASSERT_TRUE(validateWifiSsid("", err));

    err.clear();
    TEST_ASSERT_TRUE(validateWifiSsid(std::string(32, 'x'), err));

    err.clear();
    TEST_ASSERT_FALSE(validateWifiSsid(std::string(33, 'x'), err));
    TEST_ASSERT_EQUAL_STRING("wifi_ssid must be 32 characters or fewer", err.c_str());
}

void testValidateWifiPassLength() {
    std::string err;

    TEST_ASSERT_TRUE(validateWifiPass("", err));

    err.clear();
    TEST_ASSERT_TRUE(validateWifiPass(std::string(8, 'a'), err));

    err.clear();
    TEST_ASSERT_TRUE(validateWifiPass(std::string(63, 'a'), err));

    err.clear();
    TEST_ASSERT_FALSE(validateWifiPass(std::string(7, 'a'), err));
    TEST_ASSERT_EQUAL_STRING("wifi_pass must be 8-63 characters (WPA2) or empty for AP-only", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validateWifiPass(std::string(64, 'a'), err));
    TEST_ASSERT_EQUAL_STRING("wifi_pass must be 8-63 characters (WPA2) or empty for AP-only", err.c_str());
}

void testValidateAdminPasswordMin() {
    std::string err;

    TEST_ASSERT_TRUE(validateAdminPassword("", err));

    err.clear();
    TEST_ASSERT_FALSE(validateAdminPassword("short", err));

    err.clear();
    TEST_ASSERT_FALSE(validateAdminPassword("1234567", err));

    err.clear();
    TEST_ASSERT_TRUE(validateAdminPassword("12345678", err));
}

void testValidateSensorOffset() {
    std::string err;

    TEST_ASSERT_TRUE(validateSensorOffset(0.0f, err));

    err.clear();
    TEST_ASSERT_TRUE(validateSensorOffset(-50.0f, err));

    err.clear();
    TEST_ASSERT_TRUE(validateSensorOffset(50.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validateSensorOffset(-50.1f, err));
    TEST_ASSERT_EQUAL_STRING("sensor.offset_cm must be between -50 and 50", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validateSensorOffset(50.1f, err));
    TEST_ASSERT_EQUAL_STRING("sensor.offset_cm must be between -50 and 50", err.c_str());
}

void testValidateTankCalibration() {
    std::string err;

    TEST_ASSERT_TRUE(validateTankCalibration(145.0f, 10.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validateTankCalibration(10.0f, 145.0f, err));
    TEST_ASSERT_EQUAL_STRING(
        "tank.empty_distance_cm must be greater than tank.full_distance_cm",
        err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validateTankCalibration(50.0f, 50.0f, err));
}

bool validatePowerConfig(const std::string& mode, int sleepSec, int webWindowSec,
                         int wifiTimeoutMs, int wifiRetrySec, std::string& error) {
    if (mode != "normal" && mode != "battery") {
        error = "power.mode must be 'normal' or 'battery'";
        return false;
    }
    if (sleepSec < 30 || sleepSec > 86400) {
        error = "power.sleep_interval_sec must be between 30 and 86400";
        return false;
    }
    if (webWindowSec < 10 || webWindowSec > sleepSec) {
        error = "power.web_window_sec must be between 10 and sleep_interval_sec";
        return false;
    }
    if (wifiTimeoutMs < 5000 || wifiTimeoutMs > 60000) {
        error = "power.wifi_timeout_ms must be between 5000 and 60000";
        return false;
    }
    if (wifiRetrySec < 10 || wifiRetrySec > 3600) {
        error = "power.wifi_retry_interval_sec must be between 10 and 3600";
        return false;
    }
    return true;
}

void testValidatePowerConfig() {
    std::string err;

    TEST_ASSERT_TRUE(validatePowerConfig("normal", 300, 60, 20000, 120, err));

    err.clear();
    TEST_ASSERT_TRUE(validatePowerConfig("battery", 60, 30, 10000, 60, err));

    err.clear();
    TEST_ASSERT_FALSE(validatePowerConfig("turbo", 300, 60, 20000, 120, err));
    TEST_ASSERT_EQUAL_STRING("power.mode must be 'normal' or 'battery'", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validatePowerConfig("battery", 29, 10, 20000, 120, err));
    TEST_ASSERT_EQUAL_STRING("power.sleep_interval_sec must be between 30 and 86400", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validatePowerConfig("battery", 300, 301, 20000, 120, err));
    TEST_ASSERT_EQUAL_STRING("power.web_window_sec must be between 10 and sleep_interval_sec", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validatePowerConfig("battery", 300, 60, 4999, 120, err));
    TEST_ASSERT_EQUAL_STRING("power.wifi_timeout_ms must be between 5000 and 60000", err.c_str());

    err.clear();
    TEST_ASSERT_FALSE(validatePowerConfig("battery", 300, 60, 20000, 9, err));
    TEST_ASSERT_EQUAL_STRING("power.wifi_retry_interval_sec must be between 10 and 3600", err.c_str());
}
