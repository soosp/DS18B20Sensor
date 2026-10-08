/**
 * @file test_read.cpp
 * @brief Readings, error classification, counters, ranges, retries, status.
 */

#include "test_harness.h"

struct Fixture {
    OneWire           ow;
    DallasTemperature dt;
    DS18B20Sensor     s;
    uint8_t           a[8];

    explicit Fixture(uint8_t family = DS18B20MODEL) {
        makeAddress(a, 1, family);
        dt.add(a);
        s.begin(dt, ow, 0, false);
    }
    DallasTemperature::Device& dev() { return dt.dev[0]; }
};

int main() {
    printf("test_read\n");

    SECTION("valid reading");
    {
        Fixture f;
        f.dev().raw = DallasTemperature::celsiusToRaw(-17.5f);
        CHECK_NEAR(f.s.read(f.dt, 1234), -17.5f, 1e-6);
        CHECK(f.s.lastError() == Err::NONE);
        CHECK(f.s.lastValidMs() == 1234);
        CHECK(f.s.consecutiveErrors() == 0);
        CHECK_NEAR(f.s.lastTemperature(), -17.5f, 1e-6);
    }

    SECTION("each raw error code is classified");
    {
        struct { int32_t raw; Err e; } cases[] = {
            { DEVICE_DISCONNECTED_RAW,       Err::DISCONNECTED },
            { DEVICE_POWER_ON_RESET_RAW,     Err::POWER_ON_RESET },
            { DEVICE_INSUFFICIENT_POWER_RAW, Err::INSUFFICIENT_POWER },
            { DEVICE_FAULT_OPEN_RAW,         Err::FAULT },
            { DEVICE_FAULT_SHORTGND_RAW,     Err::FAULT },
            { DEVICE_FAULT_SHORTVDD_RAW,     Err::FAULT },
            { -30000,                        Err::DISCONNECTED },  // unknown code
        };
        for (auto& c : cases) {
            Fixture f;
            f.dev().raw = c.raw;
            CHECK(std::isnan(f.s.read(f.dt, 0)));
            CHECK(f.s.lastError() == c.e);
            CHECK(DS18B20Sensor::classifyRaw(c.raw) == c.e);
        }
        CHECK(DS18B20Sensor::classifyRaw(DEVICE_DISCONNECTED_RAW + 1) == Err::NONE);
    }

    SECTION("absent device: DISCONNECTED");
    {
        Fixture f;
        f.dev().present = false;
        CHECK(std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.s.lastError() == Err::DISCONNECTED);
    }

    SECTION("85 C after a reset is an error, a real 85 C is not");
    {
        Fixture f;
        f.dev().raw = DEVICE_POWER_ON_RESET_RAW;   // what 4.x reports for the POR pattern
        CHECK(std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.s.lastError() == Err::POWER_ON_RESET);
        f.dev().raw = DallasTemperature::celsiusToRaw(85.0f);
        CHECK_NEAR(f.s.read(f.dt, 0), 85.0f, 1e-6);
    }

    SECTION("range: DS18x20 up to 125, and the -55 C limit of the error encoding");
    {
        Fixture f;
        f.dev().raw = DallasTemperature::celsiusToRaw(-54.9375f);  // lowest step above -55
        CHECK(!std::isnan(f.s.read(f.dt, 0)));
        f.dev().raw = DallasTemperature::celsiusToRaw(125.0f);
        CHECK(!std::isnan(f.s.read(f.dt, 0)));
        // -55 C is raw -7040 == DEVICE_DISCONNECTED_RAW: indistinguishable
        CHECK(DallasTemperature::celsiusToRaw(-55.0f) == DEVICE_DISCONNECTED_RAW);
        f.dev().raw = DallasTemperature::celsiusToRaw(-55.0f);
        CHECK(std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.s.lastError() == Err::DISCONNECTED);
        f.dev().raw = DallasTemperature::celsiusToRaw(126.0f);
        CHECK(std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.s.lastError() == Err::OUT_OF_RANGE);
    }

    SECTION("range: MAX31850 (family 0x3B) is a thermocouple");
    {
        Fixture f(DS1825MODEL);
        f.dev().raw = DallasTemperature::celsiusToRaw(800.0f);
        CHECK_NEAR(f.s.read(f.dt, 0), 800.0f, 1e-3);
        f.dev().raw = DallasTemperature::celsiusToRaw(-54.0f);
        CHECK(!std::isnan(f.s.read(f.dt, 0)));
    }

    SECTION("consecutive errors count up and reset on a valid read");
    {
        Fixture f;
        f.dev().raw = DallasTemperature::celsiusToRaw(-18.0f);
        f.s.read(f.dt, 1000);
        f.dev().present = false;
        f.s.read(f.dt, 2000);
        f.s.read(f.dt, 3000);
        f.s.read(f.dt, 4000);
        CHECK(f.s.consecutiveErrors() == 3);
        CHECK(f.s.lastValidMs() == 1000);           // not moved by failures
        CHECK(std::isnan(f.s.lastTemperature()));
        f.dev().present = true;
        f.s.read(f.dt, 5000);
        CHECK(f.s.consecutiveErrors() == 0);
        CHECK(f.s.lastValidMs() == 5000);
    }

    SECTION("consecutive errors saturate");
    {
        Fixture f;
        f.dev().present = false;
        for (uint32_t i = 0; i < 70000; i++) f.s.read(f.dt, i);
        CHECK(f.s.consecutiveErrors() == 0xFFFF);
    }

    SECTION("CRC retries");
    {
        Fixture f;
        f.dev().raw = DallasTemperature::celsiusToRaw(-18.0f);
        f.dev().crcFails = 1;                       // default: 1 retry
        CHECK(!std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.dt.lastRetryCount == DS18B20Sensor::DEFAULT_READ_RETRIES);

        f.s.setReadRetries(0);
        f.dev().crcFails = 1;
        CHECK(std::isnan(f.s.read(f.dt, 0)));
        CHECK(f.s.lastError() == Err::DISCONNECTED);

        f.s.setReadRetries(3);
        f.dev().crcFails = 3;
        CHECK(!std::isnan(f.s.read(f.dt, 0)));
    }

    SECTION("getStatus() matches the getters");
    {
        Fixture f;
        f.dev().raw = DallasTemperature::celsiusToRaw(4.25f);
        f.s.read(f.dt, 777);
        DS18B20Sensor::Status st = f.s.getStatus();
        CHECK(memcmp(st.address, f.a, 8) == 0);
        CHECK(st.initialised);
        CHECK(!st.identified);
        CHECK(st.family == CheckDS18B20::FAMILY_UNKNOWN);
        CHECK(st.resolution == 12);
        CHECK_NEAR(st.lastTemperature, 4.25f, 1e-6);
        CHECK(st.lastError == Err::NONE);
        CHECK(st.consecutiveErrors == 0);
        CHECK(st.lastValidMs == 777);
    }

    return finish("test_read");
}
