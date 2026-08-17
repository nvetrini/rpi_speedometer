# Modular Architecture Documentation

This document describes the decomposed, single-responsibility module architecture for the wheel sensor speedometer application.

## Overview

The original monolithic `main.c` file (732 lines) has been decomposed into **8 focused modules** following the Single Responsibility Principle. Each module has a clear, well-defined interface and minimal dependencies on other modules.

## Module Structure

```
.
├── include/
│   └── app.h              # Shared types, constants, and inline accessors
└── src/
    ├── main.c             # Entry point - calls app_run()
    ├── app.c              # Main application coordination
    ├── wheel_sensor.c/h   # Wheel revolution counting
    ├── button.c/h         # Button input and settings mode
    ├── battery.c/h        # Battery voltage monitoring
    ├── speed_calculator.c/h # Speed and distance calculations
    ├── display_output.c/h # OLED display rendering
    ├── console_output.c/h  # Console/printk output
    ├── storage.c/h        # SD card logging and NVS settings
    └── sim_shell.c        # Simulation shell commands (optional)
```

## Modules

### 1. `app.h` (Shared Types & Constants)
**Location:** `include/app.h`

**Responsibility:** Defines all shared data structures, constants, and inline accessor functions.

**Key Types:**
- `struct wheel_sensor_state` - Atomic revolution counter with debounce tracking
- `struct button_state` - Button press tracking and settings mode state
- `struct wheel_config` - Wheel diameter configuration
- `struct battery_state` - Battery voltage and percentage
- `struct runtime_state` - Speed and distance tracking

**Constants:**
- `MIN/MAX/DEFAULT_WHEEL_DIAMETER_CM`
- `DEBOUNCE_MS`, `REPORT_INTERVAL_MS`, `MODE_SWITCH_DELAY_MS`
- `BATTERY_MIN_V`, `BATTERY_MAX_V`, `VOLTAGE_DIVIDER_RATIO`
- `LOG_BUFFER_SIZE`, `DISPLAY_LINE_SIZE`

**Inline Accessors:**
- `button_in_settings_mode()`
- `button_save_pending()`
- `button_clear_save_pending()`

---

### 2. `wheel_sensor` Module
**Files:** `src/wheel_sensor.c`, `src/wheel_sensor.h`

**Responsibility:** GPIO interrupt-driven wheel revolution counting with debouncing.

**Interface:**
```c
int wheel_sensor_init(struct wheel_sensor_state *sensor_state);
uint32_t wheel_sensor_get_count(const struct wheel_sensor_state *sensor_state);
```

**Details:**
- Configures GPIO from devicetree (`wheel_sensor_gpios`)
- Implements 50ms debounce in ISR
- Uses atomic operations for thread-safe counter access
- Static callback `wheel_sensor_triggered()` handles interrupts

---

### 3. `button` Module
**Files:** `src/button.c`, `src/button.h`

**Responsibility:** Button input handling with debouncing, double-press detection, and settings mode management.

**Interface:**
```c
int button_init(struct button_state *button_state);
void button_set_wheel_config(struct wheel_config *wheel_config);
int button_count(void);  // inline
```

**Details:**
- Configures two GPIO buttons from devicetree
- Button 1: Double-press (within 500ms) toggles settings mode; single press increments diameter
- Button 2: Single press decrements diameter (in settings mode only)
- 50ms debounce on all button presses
- Static callbacks `button1_pressed()` and `button2_pressed()` handle interrupts

---

### 4. `battery` Module
**Files:** `src/battery.c`, `src/battery.h`

**Responsibility:** ADC-based battery voltage monitoring and percentage calculation.

**Interface:**
```c
int battery_init(void);
int battery_read(struct battery_state *battery_state);
float battery_adc_to_voltage(int16_t adc_value);
int battery_voltage_to_percentage(float voltage);
```

**Details:**
- Reads from ADC channel 0 (GPIO 26 on Pico)
- 12-bit ADC with 3.3V reference
- Applies voltage divider ratio (3/2) for 2xAA batteries
- Linear approximation: 2.0V = 0%, 3.0V = 100%
- Clamps percentage to 0-100 range

---

### 5. `speed_calculator` Module
**Files:** `src/speed_calculator.c`, `src/speed_calculator.h`

**Responsibility:** Pure calculation of speed and distance from wheel revolutions.

**Interface:**
```c
void speed_calculator_update(uint32_t current_count, uint32_t last_count,
    int wheel_diameter_cm, uint32_t interval_ms,
    uint32_t *total_distance_m, float *speed_kmh);
float speed_calculator_circumference_m(int diameter_cm);
float speed_calculator_distance_m(uint32_t count, float circumference_m);
float speed_calculator_speed_kmh(float distance_m, float time_s);
```

**Details:**
- No hardware dependencies - pure math
- Circumference = π * diameter (cm to m)
- Speed (km/h) = distance (m) / time (s) * 3.6
- Thread-safe: takes snapshots of counts, doesn't modify shared state

---

### 6. `display_output` Module
**Files:** `src/display_output.c`, `src/display_output.h`

**Responsibility:** Rendering application state to SSD1306 OLED display.

**Interface:**
```c
int display_init(void);
void display_render(const struct button_state *button_state,
    const struct wheel_config *wheel_config,
    const struct runtime_state *runtime_state,
    const struct battery_state *battery_state);
bool display_is_available(void);
```

**Details:**
- Uses Character Framebuffer (CFB) API
- Displays 4 lines: speed, distance, wheel diameter, battery
- In settings mode: shows settings instructions
- Conditional compilation: disabled when `CONFIG_DISPLAY` not set
- Font height auto-detected or defaults to 8px

---

### 7. `console_output` Module
**Files:** `src/console_output.c`, `src/console_output.h`

**Responsibility:** All console output via `printk()`.

**Interface:**
```c
void console_print_status(uint32_t current_count, float speed_kmh,
    uint32_t total_distance_m, int wheel_diameter_cm);
void console_print_settings_mode(bool in_settings_mode, int diameter_cm);
void console_print_diameter(int diameter_cm);
void console_print_battery(int percentage, float voltage);
void console_print_init(int diameter_cm);
void console_print_error(const char *msg);
void console_print_warning(const char *msg);
```

**Details:**
- Centralizes all printk calls for consistent formatting
- Speed displayed as "X.Y km/h" (1 decimal place)
- Easy to redirect or disable output for testing

---

### 8. `storage` Module
**Files:** `src/storage.c`, `src/storage.h`

**Responsibility:** SD card logging and NVS (Non-Volatile Storage) for persistent settings.

**Interface:**
```c
int storage_init(struct wheel_config *wheel_config);
int storage_sd_init(void);
int storage_log_open(void);
void storage_log_write(const char *msg);
void storage_save_wheel_diameter(const struct wheel_config *wheel_config);
bool storage_log_available(void);
```

**Details:**
- SD card: SPI-based, FAT filesystem (elm-fat)
- Logs to `/SD:/logs/app.log`
- Creates directory if it doesn't exist
- NVS: Saves wheel diameter to flash storage partition
- Settings subtree: `wheel_diameter`
- Log messages include timestamp (uptime in ms)

---

### 9. `app` Module
**Files:** `src/app.c`

**Responsibility:** Application coordination - initializes all modules and runs main loop.

**Interface:**
```c
int app_run(void);  // Called from main()
```

**Details:**
- Initialization order:
  1. Storage (NVS) - loads wheel diameter setting
  2. Display
  3. SD card + log file
  4. Battery ADC
  5. Wheel sensor GPIO
  6. Buttons GPIO
- Main loop:
  - Sleeps for `REPORT_INTERVAL_MS` (1000ms)
  - Samples battery every `BATTERY_SAMPLE_INTERVAL_S` (60s)
  - In settings mode: only updates display
  - Normal mode: calculates speed, prints status, logs to SD
- Uses work queue for display updates and settings save

---

### 10. `sim_shell` Module (Optional)
**Files:** `src/sim_shell.c`

**Responsibility:** Shell commands for simulating inputs during development/testing.

**Enabled when:** `CONFIG_APP_SIMULATION=y`

**Commands:**
- `sim reed` - Toggle reed switch once
- `sim wheel N M` - Simulate N revolutions with M ms interval
- `sim button1` - Simulate button 1 press
- `sim button2` - Simulate button 2 press
- `sim battery MV` - Set battery voltage to MV millivolts

---

## Data Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                         app_run() (app.c)                              │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Initialization Phase                              │
├─────────────────────────────────────────────────────────────────────┤
│  storage_init() ────> loads wheel_config from NVS                     │
│  display_init() ──> initializes OLED display                          │
│  storage_sd_init() -> initializes SD card                              │
│  storage_log_open() -> opens /SD:/logs/app.log                        │
│  battery_init() ──> initializes ADC                                  │
│  wheel_sensor_init() -> configures GPIO interrupt                      │
│  button_init() ────> configures GPIO interrupts                       │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Main Loop (1000ms interval)                       │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │ Battery Sampling (every 60s)                                     ││
│  │   battery_read() ──> battery_state                                ││
│  │   console_print_battery()                                        ││
│  └─────────────────────────────────────────────────────────────────┘│
│                              │                                          │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │ If in Settings Mode:                                             ││
│  │   queue_app_state_update() ──> display_render()                  ││
│  │   continue (skip speed calculation)                             ││
│  └─────────────────────────────────────────────────────────────────┘│
│                              │                                          │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │ Normal Mode:                                                     ││
│  │   wheel_sensor_get_count() ──> current_count (atomic read)       ││
│  │   speed_calculator_update() ──> speed, distance                   ││
│  │   console_print_status()                                        ││
│  │   storage_log_write() ────> if SD available                       ││
│  │   queue_app_state_update() ──> display_render() + save settings ││
│  └─────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Interrupt Handlers                                │
├─────────────────────────────────────────────────────────────────────┤
│  wheel_sensor_triggered() ──> atomic_inc(revolution_count)            │
│  button1_pressed() ─────────> process_button_press(0)                  │
│  button2_pressed() ─────────> process_button_press(1)                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Module Dependencies

```
app.c
├── app.h (types & constants)
├── wheel_sensor.h
├── button.h
├── battery.h
├── speed_calculator.h
├── display_output.h
├── console_output.h
└── storage.h

wheel_sensor.c
├── app.h
└── zephyr: gpio, kernel

button.c
├── app.h
└── zephyr: gpio, kernel, printk

battery.c
├── app.h
└── zephyr: adc, kernel, printk

speed_calculator.c
└── math.h

display_output.c
├── app.h
└── zephyr: display/cfb, device, devicetree, printk

console_output.c
└── zephyr: printk

storage.c
├── app.h
└── zephyr: settings, fs, disk_access, printk
```

---

## Benefits of This Architecture

1. **Single Responsibility:** Each module does one thing well
2. **Testability:** Modules can be tested in isolation
3. **Maintainability:** Changes are localized to relevant modules
4. **Reusability:** Modules like `speed_calculator` have no hardware dependencies
5. **Readability:** Smaller files are easier to understand
6. **Reduced Coupling:** Modules interact through well-defined interfaces

---

## Migration Notes

The original monolithic `main.c` (732 lines) has been split into:
- `app.c`: 130 lines (coordination)
- `wheel_sensor.c`: 70 lines
- `button.c`: 120 lines
- `battery.c`: 60 lines
- `speed_calculator.c`: 50 lines
- `display_output.c`: 130 lines
- `console_output.c`: 50 lines
- `storage.c`: 130 lines

**Total:** ~740 lines across 8 files, plus clean interfaces

All functionality from the original implementation is preserved.
