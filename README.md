# DS18B20Sensor

One DS18B20-compatible 1-Wire temperature sensor on ESP32, ESP8266 and AVR
(Arduino): its ROM address, its family (genuine Maxim or which clone), its
resolution, and readings whose failures say *why* they failed.

It is the hardware layer and nothing else. The bus objects (`OneWire`,
`DallasTemperature`) are passed in; persistence, naming, alarm logic and
locking belong to whatever owns the sensor.

## Features

- **Reads by ROM address, never by bus index.** An index is a position in a
  bus search; a search that glitches once hands back a neighbour's value under
  the wrong index. Address-based reads cannot mix sensors up.
- **Errors keep their reason.** `DallasTemperature::getTempC()` folds every
  failure into -127 °C. This library reads the raw value and classifies it, so
  the **85 °C a DS18B20 reports after a brown-out** becomes `POWER_ON_RESET`
  rather than a temperature, and weak parasite power, a missing sensor and an
  out-of-range value are told apart.
- **Clone identification** via [CheckDS18B20](https://github.com/koendv/CheckDS18B20),
  with the side effects undone: the identification writes the scratchpad, so
  `begin()` recalls it from the sensor's EEPROM and then sets the resolution
  you asked for.
- **Error accounting.** `lastError()`, a saturating `consecutiveErrors()`
  counter, and `lastValidMs()` for "no valid reading for N minutes".
- **Address helpers.** Upper-case hex formatting and CRC-checked parsing, for
  storing addresses in NVS or showing them on a web page.
- **One-call status.** `getStatus()` copies every field at once, so an owner
  holding a lock gets a consistent picture in the shortest critical section.
- **Embedded-friendly.** No dynamic allocation, no strings held (name tables
  live in flash), about 24 bytes of RAM per sensor on AVR, C++11
  (compiles under the Arduino AVR core's `-std=gnu++11`).
- **Host-testable.** The test suite runs on a PC against scriptable stubs of
  OneWire, DallasTemperature and CheckDS18B20.

## Usage

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS18B20Sensor.h>

OneWire           oneWire(4);
DallasTemperature dallas(&oneWire);
DS18B20Sensor     sensors[8];
uint8_t           count = 0;

void setup() {
    dallas.begin();
    dallas.setWaitForConversion(true);
    uint8_t found = dallas.getDS18Count();
    for (uint8_t i = 0; i < found && count < 8; i++) {
        if (sensors[count].begin(dallas, oneWire, i)) count++;   // identify, 12 bit
    }
}

void loop() {
    dallas.requestTemperatures();               // one broadcast for the whole bus
    for (uint8_t i = 0; i < count; i++) {
        float t = sensors[i].read(dallas);      // NAN on error
        if (isnan(t)) {
            char why[DS18B20Sensor::ERROR_NAME_SIZE];
            DS18B20Sensor::errorName(sensors[i].lastError(), why, sizeof(why));
            // e.g. "power_on_reset", "disconnected"
        }
    }
    delay(10000);
}
```

After a restart, attach to sensors by their stored address instead, so a
missing sensor is reported as missing rather than shifting the others:

```cpp
DeviceAddress addr;
if (DS18B20Sensor::parseAddress(storedAddress, addr)) {   // "28FF1A2B3C4D5E7A"
    if (!sensor.begin(dallas, addr)) { /* sensor is missing */ }
}
```

See `examples/ListSensors` and `examples/KnownAddress`.

## Errors

|`Error`|Meaning|
|---|---|
|`NONE`|Last read was valid|
|`NOT_INITIALISED`|`begin()` has not succeeded|
|`DISCONNECTED`|No answer, or scratchpad CRC failed after the retries|
|`POWER_ON_RESET`|The sensor reset since the conversion — the "85 °C" value|
|`INSUFFICIENT_POWER`|Conversion did not complete (weak parasite power)|
|`FAULT`|MAX31850 thermocouple open or shorted|
|`OUT_OF_RANGE`|Above the device's range (125 °C; 1800 °C for MAX31850)|

The numeric values are stable. `errorName()` gives lower-case names
(`"power_on_reset"`) suitable for logs and MQTT.

**Limitation inherited from DallasTemperature:** it encodes errors as raw
values at or below -7040, which in its 1/128 °C fixed point is exactly
-55 °C. A reading of -55 °C or below is therefore reported as `DISCONNECTED`.
That is the DS18B20's lower limit anyway, but it also cuts the MAX31850's
range.

## API

### Initialisation

|Method|Description|
|---|---|
|`bool begin(DallasTemperature&, OneWire&, uint8_t index, bool identify = true, uint8_t resolution = 12)`|Discovery: address of the `index`-th device, optional identification, scratchpad recall, resolution|
|`bool begin(DallasTemperature&, const DeviceAddress, uint8_t resolution = 12)`|Known address; no search, no identification. On failure the address is kept, so the missing sensor can still be named|

Identification uses undocumented commands and takes a few hundred
milliseconds per sensor; pass `identify = false` to skip it. It only works by
bus index, because that is CheckDS18B20's interface.

### Reading

|Method|Description|
|---|---|
|`float read(DallasTemperature&, uint32_t nowMs)`|Read after a broadcast conversion; NAN on error. `nowMs` becomes `lastValidMs` on success|
|`float read(DallasTemperature&)`|Same, at `millis()` (Arduino builds)|
|`bool isPresent(DallasTemperature&) const`|Does the sensor answer (scratchpad read, no conversion)?|
|`void setReadRetries(uint8_t)`|Scratchpad CRC retries; default 1|

### State

|Method|Description|
|---|---|
|`Status getStatus() const`|Every field below, copied at once|
|`float lastTemperature()`|Last valid reading in °C; NAN after an error|
|`Error lastError()`|Result of the last `read()`|
|`uint16_t consecutiveErrors()`|Failed reads since the last valid one; saturates at 65535|
|`uint32_t lastValidMs()`|Time of the last valid reading; 0 if none yet|
|`bool isInitialised()`|`begin()` succeeded|
|`Family family()`|CheckDS18B20 family; `FAMILY_UNKNOWN` if not identified|
|`uint8_t resolution()`|Bits, 9..12 (DS18S20: always 9)|
|`const uint8_t* address()` / `bool hasAddress(const DeviceAddress)`|ROM address|
|`char* addressStr(char*, size_t)`|Address as 16 hex digits|
|`char* familyName(char*, size_t)`|Family as text, e.g. `"C Sensylink CT1820B"`|

### Static helpers

|Function|Description|
|---|---|
|`formatAddress(const DeviceAddress, char*, size_t)`|16 upper-case hex digits; `ADDR_STR_SIZE` bytes|
|`parseAddress(const char*, DeviceAddress, bool checkCrc = true)`|16 hex digits, either case; output untouched on failure|
|`classifyRaw(int32_t)`|`Error` for a raw `DallasTemperature::getTemp()` value|
|`errorName(Error, char*, size_t)`|`ERROR_NAME_SIZE` bytes always suffice|
|`familyName(Family, char*, size_t)`|`FAMILY_NAME_SIZE` bytes always suffice|

All string outputs go into caller-supplied buffers and are always
null-terminated (truncated if the buffer is small).

### Thread safety

None, by design: drive the sensor from one task, or let the owner serialise
access. `getStatus()` exists so that owner needs only one call under its lock.

## Installation

### PlatformIO

```ini
lib_deps =
    soosp/DS18B20Sensor
```

OneWire, DallasTemperature (4.0.5 or later, for the raw error codes) and
CheckDS18B20 are installed with it.

### Arduino IDE

Search for **DS18B20Sensor** in the Library Manager; the dependencies are
offered with it.

## Host tests

`Ctrl+Shift+B` in VS Code, or see [test/README.md](test/README.md).

## License

MIT — see [LICENSE](LICENSE).
