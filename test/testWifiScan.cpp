// mergeWifiScanResult replicado desde src/main.cpp con std::string.
// Conserva la logica exacta: dedup por SSID, mantiene el RSSI mas fuerte,
// secure flag pegajoso si alguna observacion lo marca, y al llegar al
// maximo desplaza a la entrada mas debil solo si la nueva supera su RSSI.

#include <unity.h>
#include <cstdint>
#include <string>

namespace {

constexpr uint8_t WIFI_SCAN_MAX_RESULTS = 32;

struct WifiScanEntry {
    std::string ssid;
    int32_t rssi = -127;
    int32_t channel = 0;
    bool secure = false;
};

struct WifiScanState {
    uint8_t resultCount = 0;
    WifiScanEntry results[WIFI_SCAN_MAX_RESULTS];
};

WifiScanState scan;

void resetScan() {
    scan.resultCount = 0;
    for (auto& e : scan.results) e = WifiScanEntry{};
}

void mergeWifiScanResult(const std::string& ssid, int32_t rssi, int32_t channel, bool secure) {
    if (ssid.empty()) return;

    for (uint8_t i = 0; i < scan.resultCount; ++i) {
        if (scan.results[i].ssid == ssid) {
            if (rssi > scan.results[i].rssi) {
                scan.results[i].rssi = rssi;
                scan.results[i].channel = channel;
                scan.results[i].secure = secure;
            } else if (secure) {
                scan.results[i].secure = true;
            }
            return;
        }
    }

    if (scan.resultCount < WIFI_SCAN_MAX_RESULTS) {
        WifiScanEntry& entry = scan.results[scan.resultCount++];
        entry.ssid = ssid;
        entry.rssi = rssi;
        entry.channel = channel;
        entry.secure = secure;
        return;
    }

    uint8_t weakestIndex = 0;
    for (uint8_t i = 1; i < scan.resultCount; ++i) {
        if (scan.results[i].rssi < scan.results[weakestIndex].rssi) {
            weakestIndex = i;
        }
    }

    if (rssi > scan.results[weakestIndex].rssi) {
        scan.results[weakestIndex].ssid = ssid;
        scan.results[weakestIndex].rssi = rssi;
        scan.results[weakestIndex].channel = channel;
        scan.results[weakestIndex].secure = secure;
    }
}

int findIndex(const std::string& ssid) {
    for (uint8_t i = 0; i < scan.resultCount; ++i) {
        if (scan.results[i].ssid == ssid) return i;
    }
    return -1;
}

}  // namespace

void testMergeWifiScan_Dedup() {
    resetScan();
    mergeWifiScanResult("HomeWiFi", -70, 6, false);
    mergeWifiScanResult("HomeWiFi", -55, 11, true);

    TEST_ASSERT_EQUAL_UINT8(1, scan.resultCount);
    TEST_ASSERT_EQUAL_INT32(-55, scan.results[0].rssi);
    TEST_ASSERT_EQUAL_INT32(11, scan.results[0].channel);
    TEST_ASSERT_TRUE(scan.results[0].secure);
}

void testMergeWifiScan_StrongerRssiReplaces() {
    resetScan();
    mergeWifiScanResult("Office", -85, 1, true);
    mergeWifiScanResult("Office", -60, 5, false);

    TEST_ASSERT_EQUAL_UINT8(1, scan.resultCount);
    TEST_ASSERT_EQUAL_INT32(-60, scan.results[0].rssi);
    TEST_ASSERT_EQUAL_INT32(5, scan.results[0].channel);
    TEST_ASSERT_FALSE(scan.results[0].secure);
}

void testMergeWifiScan_MaxResultsKeepsStrongest() {
    resetScan();

    for (uint8_t i = 0; i < WIFI_SCAN_MAX_RESULTS; ++i) {
        std::string ssid = "net-" + std::to_string(i);
        mergeWifiScanResult(ssid, -90 + i, 1, false);
    }
    TEST_ASSERT_EQUAL_UINT8(WIFI_SCAN_MAX_RESULTS, scan.resultCount);

    mergeWifiScanResult("weak-extra", -95, 1, false);
    TEST_ASSERT_EQUAL_UINT8(WIFI_SCAN_MAX_RESULTS, scan.resultCount);
    TEST_ASSERT_EQUAL_INT(-1, findIndex("weak-extra"));

    mergeWifiScanResult("strong-extra", -40, 1, false);
    TEST_ASSERT_EQUAL_UINT8(WIFI_SCAN_MAX_RESULTS, scan.resultCount);
    TEST_ASSERT_NOT_EQUAL(-1, findIndex("strong-extra"));
    TEST_ASSERT_EQUAL_INT(-1, findIndex("net-0"));
}

void testMergeWifiScan_SecureFlagSticky() {
    resetScan();
    mergeWifiScanResult("Cafe", -70, 3, true);
    mergeWifiScanResult("Cafe", -80, 3, false);

    TEST_ASSERT_EQUAL_UINT8(1, scan.resultCount);
    TEST_ASSERT_EQUAL_INT32(-70, scan.results[0].rssi);
    TEST_ASSERT_TRUE(scan.results[0].secure);
}
