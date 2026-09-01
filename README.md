# Wheel Sensor Example (Zephyr, Raspberry Pi Pico)

Reads a reed switch or Hall-effect sensor via interrupt-driven GPIO,
counts wheel revolutions with debouncing, and prints speed/distance
over the console every second.

## Wiring

- Sensor signal leg -> GPIO 16 (change in `boards/rpi_pico.overlay`
  if you wire it to a different pin)
- Sensor other leg -> GND
- The pin is configured with an internal pull-up and treated as
  active-low, so no external resistor is required for a simple
  reed switch. If your Hall sensor has its own open-drain/open-collector
  output this also works as-is; if it has a push-pull digital output
  instead, you may want to drop the pull-up and adjust the active
  level in the overlay.

## Build

From a working Zephyr workspace (i.e. you've already done
`west init` / `west update` for the `zephyr` repo):

```
west build -b rpi_pico /path/to/wheel_sensor_app
```

## Flash

Hold the BOOTSEL button on the Pico while plugging it into USB, then:

```
west flash
```

(This mounts the Pico as a USB mass-storage device and copies the
generated UF2 image to it.)

## View output

The sample prints to the console (USB CDC-ACM or UART depending on
your prj.conf/board defaults). Connect with a serial terminal, e.g.:

```
minicom -D /dev/ttyACM0 -b 115200
```

## Notes / next steps

- `DEBOUNCE_MS` and `WHEEL_CIRCUMFERENCE_M` are the two constants
  you'll most likely want to tune first.
- This example only handles the sensor input and prints to console.
  To turn it into the full cycling computer, you'd add:
  - A display driver (e.g. SSD1306 over I2C) to show speed/distance
    instead of/in addition to printk.
  - The Settings subsystem backed by NVS to persist wheel
    circumference and units between reboots.
  - GPIO-based buttons (or the `gpio-keys` input subsystem) to adjust
    those settings at runtime.

## Simulation

- run `west build -b native_sim//64 -p` to test
- launch application with `./build/zephyr/zephyr.exe`
- attach to UART with e.g. `alacritty -e screen /dev/pts/6`

## Testing

The project includes a comprehensive unit test suite using Zephyr's **ztest** framework and **Twister** test runner.

### Test Structure

Tests are organized in the `tests/` directory with the following modules:

| Test Module | Description | Key Tests |
|-------------|-------------|-----------|
| `test_wheel_sensor` | Wheel revolution counting and sensor logic | Initialization, counting, atomic operations, debounce timing |
| `test_battery` | Battery monitoring and voltage conversion | Voltage to percentage, ADC conversion, bounds checking |
| `test_storage` | NVS settings storage | Configuration validation, persistence simulation, error handling |
| `test_sd_card` | SD card filesystem operations | Mounting, file creation, read/write operations, error codes |
| `test_speed_calculator` | Speed and distance calculations | Zero/constant/increasing revolutions, different diameters and intervals |
| `test_display` | Display rendering and state management | Initialization, rendering modes, data validation |

### Running Tests

#### Method 1: Using West (Recommended)

```bash
# Run from application root directory
cd /path/to/rpi_speedometer

# Run all tests on native_sim/native/64 (64-bit)
west twister -p . -T tests --platform native_sim/native/64

# Run specific test by name pattern
west twister -p . -T tests --platform native_sim/native/64 --test-pattern "test_wheel_sensor.*"

# Run with verbose output
west twister -p . -T tests --platform native_sim/native/64 -v

# Run with custom configuration
west twister -p . -T tests --platform native_sim/native/64 -c tests/twister.yml
```

#### Method 2: Using Twister Directly

```bash
# Run from application root directory
cd /path/to/rpi_speedometer

# Run all tests
twister -p . -T tests --platform native_sim/native/64

# Run specific test by name pattern
twister -p . -T tests --platform native_sim/native/64 --test-pattern "test_wheel_sensor.*"

# Run with custom YAML config
twister -p . -T tests --platform native_sim/native/64 -c tests/twister.yml
```

#### Method 3: Manual Build and Run

```bash
# Build and run all tests
west build -b native_sim/native/64 -P tests .

# Run specific test executable
./build/tests/test_wheel_sensor/zephyr/zephyr.exe
```

### Test Configuration

- **Test Framework**: Zephyr ztest
- **Test Runner**: Twister
- **Platform**: `native_sim/native/64` (64-bit native simulation with LP64 ABI)
- **Configuration**: `tests/prj_test.conf`
- **Twister Config**: `tests/twister.yml`

### Writing New Tests

1. Create a new test directory under `tests/test_<module>/`
2. Add a `CMakeLists.txt` file that includes your test source files
3. Create test source files using the `ZTEST()` macro
4. Add mock implementations for hardware dependencies in `src/mocks.c`
5. Update `tests/CMakeLists.txt` to include your test module
6. Add test configuration to `tests/prj_test.conf` and `tests/twister.yml`

### Test Examples

```c
#include <ztest.h>
#include "module.h"

ZTEST(module_test, test_functionality)
{
    // Test setup
    int result = function_under_test();
    
    // Assertions
    zassert_equal(result, expected_value, "Function should return expected value");
    zassert_true(condition, "Condition should be true");
}

ZTEST_SUITE(module_test, NULL, setup, NULL, NULL, teardown);
```

### Mocking Hardware Dependencies

Each test module includes a `mocks.c` file that provides mock implementations for:
- GPIO operations
- ADC readings  
- Filesystem operations
- Display functions
- Device readiness checks

Use the mock functions to simulate both success and failure scenarios.

### Code Coverage

The test suite includes **code coverage support** using gcov/lcov/genhtml. Coverage is configured in `tests/twister.yml`.

#### Coverage Commands

```bash
# Run tests with coverage and generate HTML report
./tests/build_tests.sh coverage

# Open coverage report in browser
./tests/build_tests.sh coverage-open

# Manual coverage generation (if needed)
cd build/tests
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

#### Coverage Requirements

- **gcov**: Usually comes with GCC
- **lcov**: Install with `sudo apt-get install lcov` (Ubuntu/Debian)
- **genhtml**: Part of lcov package

#### Coverage Configuration

The coverage configuration in `tests/twister.yml` includes:
- **Tool**: gcov
- **Reports**: HTML and XML formats
- **Output**: `coverage_results/` directory
- **Thresholds**: 80% line, 70% branch, 85% function coverage
- **Exclusions**: Test files and mocks are excluded from coverage

#### Coverage Targets

Coverage is measured for:
- `src/` - All source files
- `include/` - All header files
- Excludes test files and mock implementations
