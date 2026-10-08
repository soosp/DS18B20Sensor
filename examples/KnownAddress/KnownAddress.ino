/**
 * DS18B20Sensor — KnownAddress
 *
 * Attaches to sensors by stored ROM address rather than by bus position, the
 * way an application restores its configuration after a restart. A sensor
 * that is missing is reported by name instead of silently shifting the
 * others into its place.
 *
 * Replace the addresses below with your own (ListSensors prints them).
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS18B20Sensor.h>

#ifndef ONEWIRE_PIN
#define ONEWIRE_PIN 4
#endif

struct Channel {
    const char*   name;
    const char*   address;   // 16 hex digits
    DS18B20Sensor sensor;
};

Channel channels[] = {
    { "Freezer", "28FF1A2B3C4D5E7A", DS18B20Sensor() },
    { "Chiller", "28FF6B5A4C3D2E1F", DS18B20Sensor() },
};
static const uint8_t CHANNEL_COUNT = sizeof(channels) / sizeof(channels[0]);

OneWire           oneWire(ONEWIRE_PIN);
DallasTemperature dallas(&oneWire);

void setup() {
    Serial.begin(115200);
    delay(1000);
    dallas.begin();
    dallas.setWaitForConversion(true);

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        DeviceAddress addr;
        Serial.print(channels[i].name);
        if (!DS18B20Sensor::parseAddress(channels[i].address, addr)) {
            Serial.println(F(": invalid address (format or CRC)"));
            continue;
        }
        if (channels[i].sensor.begin(dallas, addr)) {
            Serial.println(F(": found"));
        } else {
            Serial.println(F(": MISSING"));
        }
    }
}

void loop() {
    dallas.requestTemperatures();
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        DS18B20Sensor::Status s = channels[i].sensor.getStatus();
        if (!s.initialised) continue;
        float t = channels[i].sensor.read(dallas);
        Serial.print(channels[i].name);
        Serial.print(F(": "));
        if (isnan(t)) Serial.println(F("--"));
        else          Serial.println(t, 1);
    }
    delay(10000);
}
