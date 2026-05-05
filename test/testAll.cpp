#include <unity.h>

extern void testValidateDeviceNameLength();
extern void testValidateWifiSsidLength();
extern void testValidateWifiPassLength();
extern void testValidateAdminPasswordMin();
extern void testValidatePowerConfig();
extern void testValidateSensorOffset();
extern void testValidateTankCalibration();
extern void testMergeWifiScan_Dedup();
extern void testMergeWifiScan_StrongerRssiReplaces();
extern void testMergeWifiScan_MaxResultsKeepsStrongest();
extern void testMergeWifiScan_SecureFlagSticky();
extern void testTrimCopy_StripsWhitespace();
extern void testDeriveDefaultPassword_Format();
extern void testSecretFallback_KeyMissing();
extern void testSecretFallback_NotReady();
extern void testAdminPassword_RejectsShort();
extern void testAdminPassword_RejectsTooLong();
extern void testAdminPassword_RejectsSameAsCurrent();
extern void testAdminPassword_RequiresCurrentWhenConfigured();
extern void testAdminPassword_AllowsFirstSetWhenUnconfigured();

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testValidateDeviceNameLength);
    RUN_TEST(testValidateWifiSsidLength);
    RUN_TEST(testValidateWifiPassLength);
    RUN_TEST(testValidateAdminPasswordMin);
    RUN_TEST(testValidatePowerConfig);
    RUN_TEST(testValidateSensorOffset);
    RUN_TEST(testValidateTankCalibration);
    RUN_TEST(testMergeWifiScan_Dedup);
    RUN_TEST(testMergeWifiScan_StrongerRssiReplaces);
    RUN_TEST(testMergeWifiScan_MaxResultsKeepsStrongest);
    RUN_TEST(testMergeWifiScan_SecureFlagSticky);
    RUN_TEST(testTrimCopy_StripsWhitespace);
    RUN_TEST(testDeriveDefaultPassword_Format);
    RUN_TEST(testSecretFallback_KeyMissing);
    RUN_TEST(testSecretFallback_NotReady);
    RUN_TEST(testAdminPassword_RejectsShort);
    RUN_TEST(testAdminPassword_RejectsTooLong);
    RUN_TEST(testAdminPassword_RejectsSameAsCurrent);
    RUN_TEST(testAdminPassword_RequiresCurrentWhenConfigured);
    RUN_TEST(testAdminPassword_AllowsFirstSetWhenUnconfigured);
    return UNITY_END();
}
