/**
 * DS18B20Sensor — ListSensors
 *
 * Finds every sensor on the bus, identifies each one (genuine Maxim or which
 * clone), then reads them all every 10 seconds with one broadcast conversion.
 * Failed readings are printed with their reason.
 *
 * Wiring: DS18B20 data line to ONEWIRE_PIN with a 4.7 kΩ pull-up to 3.3 V/5 V.
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS18B20Sensor.h>

#ifndef ONEWIRE_PIN
#define ONEWIRE_PIN 4
#endif

static const uint8_t  MAX_SENSORS        = 8;
static const uint32_t SAMPLE_INTERVAL_MS = 10000;

OneWire           oneWire(ONEWIRE_PIN);
DallasTemperature dallas(&oneWire);
DS18B20Sensor     sensors[MAX_SENSORS];
uint8_t           sensorCount = 0;

void printSensor(const DS18B20Sensor& s) {
    char addr[DS18B20Sensor::ADDR_STR_SIZE];
    char family[DS18B20Sensor::FAMILY_NAME_SIZE];
    Serial.print(s.addressStr(addr, sizeof(addr)));
    Serial.print(F("  "));
    Serial.print(s.familyName(family, sizeof(family)));
    Serial.print(F("  "));
    Serial.print(s.resolution());
    Serial.println(F(" bit"));
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    dallas.begin();
    // read() follows a completed conversion; the blocking wait keeps the
    // example simple. A real application can poll isConversionComplete().
    dallas.setWaitForConversion(true);

    uint8_t found = dallas.getDS18Count();
    Serial.print(F("Devices on the bus: "));
    Serial.println(found);

    for (uint8_t i = 0; i < found && sensorCount < MAX_SENSORS; i++) {
        if (sensors[sensorCount].begin(dallas, oneWire, i)) {
            printSensor(sensors[sensorCount]);
            sensorCount++;
        }
    }
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last < SAMPLE_INTERVAL_MS) return;
    last = millis();

    dallas.requestTemperatures();          // all sensors convert at once

    char addr[DS18B20Sensor::ADDR_STR_SIZE];
    char err[DS18B20Sensor::ERROR_NAME_SIZE];
    for (uint8_t i = 0; i < sensorCount; i++) {
        float t = sensors[i].read(dallas);
        Serial.print(sensors[i].addressStr(addr, sizeof(addr)));
        Serial.print(F(": "));
        if (isnan(t)) {
            Serial.print(F("error "));
            Serial.print(DS18B20Sensor::errorName(sensors[i].lastError(), err, sizeof(err)));
            Serial.print(F(" (x"));
            Serial.print(sensors[i].consecutiveErrors());
            Serial.println(F(")"));
        } else {
            Serial.print(t, 2);
            Serial.println(F(" C"));
        }
    }
    Serial.println();
}
