/**
 * @file test_helpers.cpp
 * @brief Address formatting/parsing and the name helpers.
 */

#include "test_harness.h"

int main() {
    printf("test_helpers\n");

    SECTION("formatAddress: upper-case hex, truncation, null buffer");
    {
        const DeviceAddress a = { 0x28, 0xFF, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F };
        char buf[DS18B20Sensor::ADDR_STR_SIZE];
        CHECK(strcmp(DS18B20Sensor::formatAddress(a, buf, sizeof(buf)), "28FF1A2B3C4D5E6F") == 0);
        char small[6];
        CHECK(strcmp(DS18B20Sensor::formatAddress(a, small, sizeof(small)), "28FF1") == 0);
        CHECK(DS18B20Sensor::formatAddress(a, nullptr, 17) == nullptr);
        CHECK(DS18B20Sensor::formatAddress(a, buf, 0) == nullptr);
    }

    SECTION("parseAddress: round trip, either case");
    {
        uint8_t a[8], b[8];
        makeAddress(a, 9);
        char buf[DS18B20Sensor::ADDR_STR_SIZE];
        DS18B20Sensor::formatAddress(a, buf, sizeof(buf));
        CHECK(DS18B20Sensor::parseAddress(buf, b));
        CHECK(memcmp(a, b, 8) == 0);
        for (char* p = buf; *p; p++) if (*p >= 'A' && *p <= 'F') *p = (char)(*p - 'A' + 'a');
        memset(b, 0, 8);
        CHECK(DS18B20Sensor::parseAddress(buf, b));
        CHECK(memcmp(a, b, 8) == 0);
    }

    SECTION("parseAddress: rejects bad input and leaves the output alone");
    {
        uint8_t out[8];
        memset(out, 0xEE, 8);
        CHECK(!DS18B20Sensor::parseAddress(nullptr, out));
        CHECK(!DS18B20Sensor::parseAddress("", out));
        CHECK(!DS18B20Sensor::parseAddress("28FF1A2B3C4D5E6", out));     // 15 digits
        CHECK(!DS18B20Sensor::parseAddress("28FF1A2B3C4D5E6F0", out));   // 17 digits
        CHECK(!DS18B20Sensor::parseAddress("28FF1A2B3C4D5E6G", out));    // not hex
        CHECK(!DS18B20Sensor::parseAddress("28FF1A2B3C4D5E6F", out));    // CRC wrong
        CHECK(DS18B20Sensor::parseAddress("28FF1A2B3C4D5E6F", out, false));
        uint8_t untouched[8];
        memset(untouched, 0xEE, 8);
        uint8_t out2[8];
        memset(out2, 0xEE, 8);
        DS18B20Sensor::parseAddress("zz", out2);
        CHECK(memcmp(out2, untouched, 8) == 0);
    }

    SECTION("errorName");
    {
        char buf[DS18B20Sensor::ERROR_NAME_SIZE];
        CHECK(strcmp(DS18B20Sensor::errorName(Err::NONE, buf, sizeof(buf)), "none") == 0);
        CHECK(strcmp(DS18B20Sensor::errorName(Err::POWER_ON_RESET, buf, sizeof(buf)), "power_on_reset") == 0);
        size_t longest = 0;
        for (uint8_t i = 0; i < DS18B20Sensor::ERROR_COUNT; i++)
            longest = std::max(longest, strlen(DS18B20Sensor::errorName((Err)i, buf, sizeof(buf))));
        CHECK(longest == DS18B20Sensor::ERROR_NAME_LEN);
        CHECK(strcmp(DS18B20Sensor::errorName((Err)99, buf, sizeof(buf)), "disconnected") == 0);
        CHECK(DS18B20Sensor::errorName(Err::NONE, nullptr, 4) == nullptr);
    }

    SECTION("familyName");
    {
        char buf[DS18B20Sensor::FAMILY_NAME_SIZE];
        CHECK(strcmp(DS18B20Sensor::familyName(CheckDS18B20::FAMILY_A1, buf, sizeof(buf)), "A1 Maxim DS18B20") == 0);
        CHECK(strcmp(DS18B20Sensor::familyName(CheckDS18B20::FAMILY_UNKNOWN, buf, sizeof(buf)), "?") == 0);
        CHECK(strcmp(DS18B20Sensor::familyName(CheckDS18B20::NO_SENSOR, buf, sizeof(buf)), "no sensor") == 0);
        CHECK(strcmp(DS18B20Sensor::familyName((DS18B20Sensor::Family)99, buf, sizeof(buf)), "?") == 0);
        size_t longest = 0;
        for (int i = 0; i <= CheckDS18B20::NO_SENSOR; i++)
            longest = std::max(longest, strlen(DS18B20Sensor::familyName((DS18B20Sensor::Family)i, buf, sizeof(buf))));
        CHECK(longest == DS18B20Sensor::FAMILY_NAME_LEN);
        char small[3];
        CHECK(strcmp(DS18B20Sensor::familyName(CheckDS18B20::FAMILY_C, small, sizeof(small)), "C ") == 0);
    }

    return finish("test_helpers");
}
