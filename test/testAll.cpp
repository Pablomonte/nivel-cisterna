#include <unity.h>

extern void testValidateDeviceNameLength();
extern void testValidateWifiSsidLength();
extern void testValidatePumpThresholds();
extern void testValidateAdminPasswordMin();
extern void testMergeWifiScan_Dedup();
extern void testMergeWifiScan_StrongerRssiReplaces();
extern void testMergeWifiScan_MaxResultsKeepsStrongest();
extern void testMergeWifiScan_SecureFlagSticky();
extern void testTrimCopy_StripsWhitespace();
extern void testDeriveDefaultPassword_Format();
extern void testSecretFallback_KeyMissing();
extern void testSecretFallback_NotReady();

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testValidateDeviceNameLength);
    RUN_TEST(testValidateWifiSsidLength);
    RUN_TEST(testValidatePumpThresholds);
    RUN_TEST(testValidateAdminPasswordMin);
    RUN_TEST(testMergeWifiScan_Dedup);
    RUN_TEST(testMergeWifiScan_StrongerRssiReplaces);
    RUN_TEST(testMergeWifiScan_MaxResultsKeepsStrongest);
    RUN_TEST(testMergeWifiScan_SecureFlagSticky);
    RUN_TEST(testTrimCopy_StripsWhitespace);
    RUN_TEST(testDeriveDefaultPassword_Format);
    RUN_TEST(testSecretFallback_KeyMissing);
    RUN_TEST(testSecretFallback_NotReady);
    return UNITY_END();
}
