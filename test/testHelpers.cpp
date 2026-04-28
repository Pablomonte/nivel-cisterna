// Helpers replicados desde src/main.cpp:
//   - trimCopy: quita whitespace (Arduino String::trim equivalente).
//   - deriveDefaultPassword: "cisterna-XXXXXX" con los 24 bits bajos del MAC,
//     hex en mayusculas, ancho fijo 6.

#include <unity.h>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {

std::string trimCopy(std::string value) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value.empty() && isSpace(value.front())) value.erase(value.begin());
    while (!value.empty() && isSpace(value.back())) value.pop_back();
    return value;
}

std::string deriveDefaultPassword(uint64_t mac) {
    char buf[24];
    snprintf(buf, sizeof(buf), "cisterna-%06llX",
             static_cast<unsigned long long>(mac & 0xFFFFFFULL));
    return std::string(buf);
}

}  // namespace

void testTrimCopy_StripsWhitespace() {
    TEST_ASSERT_EQUAL_STRING("hi", trimCopy("  hi  ").c_str());
    TEST_ASSERT_EQUAL_STRING("", trimCopy("").c_str());
    TEST_ASSERT_EQUAL_STRING("", trimCopy("   ").c_str());
    TEST_ASSERT_EQUAL_STRING("x", trimCopy("x").c_str());
    TEST_ASSERT_EQUAL_STRING("a b", trimCopy("\t a b \n").c_str());
}

void testDeriveDefaultPassword_Format() {
    TEST_ASSERT_EQUAL_STRING("cisterna-123456", deriveDefaultPassword(0x123456ULL).c_str());
    TEST_ASSERT_EQUAL_STRING("cisterna-123456", deriveDefaultPassword(0xABCDEF123456ULL).c_str());
    TEST_ASSERT_EQUAL_STRING("cisterna-000001", deriveDefaultPassword(0x1ULL).c_str());
    TEST_ASSERT_EQUAL_STRING("cisterna-FFFFFF", deriveDefaultPassword(0xFFFFFFULL).c_str());
}
