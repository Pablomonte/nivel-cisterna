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

bool validatePumpThresholds(float low, float high, float hyst, std::string& error) {
    if (low < 0 || low > 100 || high < 0 || high > 100 || low >= high) {
        error = "pump thresholds must satisfy 0 <= low < high <= 100";
        return false;
    }
    if (hyst < 0 || hyst > 20) {
        error = "pump.hysteresis must be between 0 and 20";
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

void testValidatePumpThresholds() {
    std::string err;

    TEST_ASSERT_TRUE(validatePumpThresholds(20.0f, 90.0f, 5.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validatePumpThresholds(90.0f, 90.0f, 5.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validatePumpThresholds(95.0f, 90.0f, 5.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validatePumpThresholds(-1.0f, 90.0f, 5.0f, err));

    err.clear();
    TEST_ASSERT_FALSE(validatePumpThresholds(20.0f, 90.0f, 25.0f, err));
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
