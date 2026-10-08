# Host tests

`DS18B20Sensor` takes the bus objects as parameters and, without `ARDUINO`
defined, compiles as plain C++. The tests replace the bus with the stubs in
`stubs/`: no board, no sensors, no real time.

## Running

Press `Ctrl+Shift+B` in VS Code — the default build task builds and runs every
test and reports a summary.

From a shell:

```sh
bash test/run_tests.sh   # Linux / macOS
test\run_tests.bat       # Windows
```

Both scripts exit non-zero if any test fails. They need nothing but a C++17
compiler (`g++` by default; override with `CXX`).

On Windows the VSCode task prepends `C:\msys64\ucrt64\bin` to `PATH` so the
MSYS2 toolchain is found without changing the system PATH. If MSYS2 lives
elsewhere, edit that entry in `.vscode/tasks.json`.

## What is covered

|File|Area|
|---|---|
|`test_begin.cpp`|Begin by index and by address, identification and scratchpad recall, resolution, missing devices|
|`test_read.cpp`|Valid readings, every raw error code, ranges, error counter, CRC retries, `getStatus()`|
|`test_helpers.cpp`|Address formatting and parsing (CRC, case, length), error and family names|

## stubs/

|File|Stands in for|
|---|---|
|`OneWire.h`|OneWire: the class and the real CRC-8|
|`DallasTemperature.h`|DallasTemperature 4.x: a device table whose raw readings, presence, CRC failures and resolution the tests script|
|`CheckDS18B20.h`|CheckDS18B20: the family enum and a scriptable `ds18b20_family()`|

The stubs implement only what the library and the examples call. The
constants (`DEVICE_*_RAW`, family codes) are copied from DallasTemperature
4.0.x, so classification is tested against the real values.
