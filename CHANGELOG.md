# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-10-08

### Added

- `DS18B20Sensor`: one DS18B20-compatible 1-Wire sensor
  - `begin()` by bus index (with optional CheckDS18B20 identification,
    scratchpad recall and resolution) or by known ROM address
  - `read()` by ROM address after a broadcast conversion, with an
    explicit-time overload for host tests
  - raw error classification: `DISCONNECTED`, `POWER_ON_RESET`,
    `INSUFFICIENT_POWER`, `FAULT`, `OUT_OF_RANGE`, `NOT_INITIALISED`
  - `lastError()`, saturating `consecutiveErrors()`, `lastValidMs()`
  - `getStatus()`: every field in one copy
  - `formatAddress()`, CRC-checked `parseAddress()`, `errorName()`,
    `familyName()`
- Host test suite in `test/` with stubs of OneWire, DallasTemperature and
  CheckDS18B20, runnable from VS Code with Ctrl+Shift+B
- Examples: `ListSensors`, `KnownAddress`

[Unreleased]: https://github.com/soosp/DS18B20Sensor/compare/0.1.0...HEAD
[0.1.0]: https://github.com/soosp/DS18B20Sensor/releases/tag/0.1.0
