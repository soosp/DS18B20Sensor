/**
 * @file test_begin.cpp
 * @brief Initialisation by bus index and by address.
 */

#include "test_harness.h"

int main() {
    printf("test_begin\n");
    OneWire ow;

    SECTION("before begin(): not initialised, read() fails");
    {
        DallasTemperature dt;
        DS18B20Sensor s;
        CHECK(!s.isInitialised());
        CHECK(s.lastError() == Err::NOT_INITIALISED);
        CHECK(std::isnan(s.read(dt, 0)));
        CHECK(s.lastError() == Err::NOT_INITIALISED);
        CHECK(dt.getTempCalls == 0);
    }

    SECTION("begin by index: address, identification, scratchpad recall, resolution");
    {
        DallasTemperature dt;
        uint8_t a[8], b[8];
        makeAddress(a, 1); makeAddress(b, 2);
        dt.add(a).resolution = 9;
        dt.add(b);
        CheckDS18B20::stubFamilies()[1] = CheckDS18B20::FAMILY_C;
        CheckDS18B20::stubCalls() = 0;

        DS18B20Sensor s;
        CHECK(s.begin(dt, ow, 1));
        CHECK(s.isInitialised());
        CHECK(s.hasAddress(b));
        CHECK(s.family() == CheckDS18B20::FAMILY_C);
        CHECK(s.getStatus().identified);
        CHECK(CheckDS18B20::stubCalls() == 1);
        CHECK(dt.recallCalls == 1);                 // identification is undone
        CHECK(s.resolution() == 12);
        CHECK(dt.dev[1].resolution == 12);
        CHECK(s.lastError() == Err::NONE);
        CheckDS18B20::stubFamilies()[1] = CheckDS18B20::FAMILY_A1;
    }

    SECTION("begin by index without identification");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 1);
        dt.add(a);
        CheckDS18B20::stubCalls() = 0;
        DS18B20Sensor s;
        CHECK(s.begin(dt, ow, 0, false, 10));
        CHECK(CheckDS18B20::stubCalls() == 0);
        CHECK(dt.recallCalls == 0);
        CHECK(s.family() == CheckDS18B20::FAMILY_UNKNOWN);
        CHECK(!s.getStatus().identified);
        CHECK(s.resolution() == 10);
    }

    SECTION("begin by index: nothing at that index");
    {
        DallasTemperature dt;
        DS18B20Sensor s;
        CHECK(!s.begin(dt, ow, 3));
        CHECK(!s.isInitialised());
    }

    SECTION("resolution is clamped to 9..12");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 1);
        dt.add(a);
        DS18B20Sensor s;
        CHECK(s.begin(dt, ow, 0, false, 4));
        CHECK(s.resolution() == 9);
        CHECK(s.begin(dt, ow, 0, false, 15));
        CHECK(s.resolution() == 12);
    }

    SECTION("DS18S20 reports its fixed 9-bit resolution");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 1, DS18S20MODEL);
        dt.add(a);
        DS18B20Sensor s;
        CHECK(s.begin(dt, ow, 0, false, 12));
        CHECK(s.resolution() == 9);
    }

    SECTION("setResolution failure fails begin()");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 1);
        dt.add(a).setResolutionFails = true;
        DS18B20Sensor s;
        CHECK(!s.begin(dt, ow, 0, false));
        CHECK(!s.isInitialised());
    }

    SECTION("begin by address: present");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 5);
        dt.add(a);
        DS18B20Sensor s;
        CHECK(s.begin(dt, a));
        CHECK(s.isInitialised());
        CHECK(s.hasAddress(a));
        CHECK(!s.getStatus().identified);
        CHECK(s.isPresent(dt));
    }

    SECTION("begin by address: missing device keeps the address");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 5);
        DS18B20Sensor s;
        CHECK(!s.begin(dt, a));
        CHECK(!s.isInitialised());
        CHECK(s.hasAddress(a));
        char buf[DS18B20Sensor::ADDR_STR_SIZE];
        char expect[DS18B20Sensor::ADDR_STR_SIZE];
        DS18B20Sensor::formatAddress(a, expect, sizeof(expect));
        CHECK(strcmp(s.addressStr(buf, sizeof(buf)), expect) == 0);
        CHECK(!s.isPresent(dt));
    }

    SECTION("re-begin clears the previous state");
    {
        DallasTemperature dt;
        uint8_t a[8];
        makeAddress(a, 1);
        dt.add(a).raw = DallasTemperature::celsiusToRaw(-18.0f);
        DS18B20Sensor s;
        s.begin(dt, ow, 0, false);
        s.read(dt, 1000);
        dt.dev[0].present = false;
        s.read(dt, 2000);
        CHECK(s.consecutiveErrors() == 1);
        dt.dev[0].present = true;
        CHECK(s.begin(dt, ow, 0, false));
        CHECK(s.consecutiveErrors() == 0);
        CHECK(s.lastValidMs() == 0);
        CHECK(std::isnan(s.lastTemperature()));
    }

    return finish("test_begin");
}
