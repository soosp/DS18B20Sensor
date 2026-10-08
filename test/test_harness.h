#pragma once

/**
 * @file test_harness.h
 * @brief Assertion helpers and fixtures for the DS18B20Sensor host tests.
 *
 * The stubs in stubs/ stand in for OneWire, DallasTemperature and
 * CheckDS18B20: a scriptable device table instead of a bus. Without ARDUINO
 * defined, DS18B20Sensor.h compiles as plain C++.
 */

#include "../src/DS18B20Sensor.h"
#include <cstdio>
#include <cmath>
#include <cstring>

using Err = DS18B20Sensor::Error;

inline int& testFailures() { static int failures = 0; return failures; }

inline void checkImpl(bool ok, const char* expr, const char* file, int line) {
    if (!ok) {
        testFailures()++;
        printf("  FAIL %s:%d  %s\n", file, line, expr);
    }
}

#define CHECK(expr) checkImpl((expr), #expr, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) checkImpl(std::fabs((double)(a) - (double)(b)) <= (tol), \
                                        #a " ~= " #b, __FILE__, __LINE__)
#define SECTION(name) printf("- %s\n", name)

inline int finish(const char* name) {
    if (testFailures() == 0) printf("%s: OK\n", name);
    else                     printf("%s: %d FAILED\n", name, testFailures());
    return testFailures() == 0 ? 0 : 1;
}

/// A valid DS18B20 ROM address (family 0x28, correct CRC) with serial @p n.
inline void makeAddress(uint8_t* a, uint8_t n, uint8_t family = DS18B20MODEL) {
    a[0] = family;
    for (uint8_t i = 1; i < 7; i++) a[i] = (uint8_t)(n * 17 + i);
    a[7] = OneWire::crc8(a, 7);
}
