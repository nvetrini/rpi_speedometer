# AGENTS.md - Wheel Sensor Speedometer (Zephyr RTOS / Raspberry Pi Pico)

## Project Overview

This repository implements a **bike computer/speedometer** application using the **Zephyr RTOS** targeting the **Raspberry Pi Pico** (RP2040). The application reads wheel revolutions from a sensor, calculates speed and distance, and provides user interface and logging capabilities.

**Primary Purpose:** Embedded real-time wheel speed and distance monitoring with user-configurable wheel size.

## Architecture

### Hardware Target
- **Primary:** Raspberry Pi Pico (RP2040 dual-core Cortex-M0+)
- **Supported:** Native POSIX simulation (`native_sim//64`) for development/testing

### Core Components

| Component | Location | Purpose |
|-----------|----------|---------|
| Main Application | `src/main.c` | Core logic: sensor input, speed/distance calculation, state management |
| Simulation Shell | `src/sim_shell.c` | Shell commands for simulating inputs during testing |
| Board Overlay (Pico) | `boards/rpi_pico.overlay` | Devicetree configuration for GPIO, ADC, SPI, SD card |
| Board Overlay (Sim) | `boards/native_sim_64.overlay` | Devicetree for native simulation |
| Simulation Config | `boards/native_sim.conf` | Kconfig overrides for simulation builds |
| Project Config | `prj.conf` | Zephyr configuration options |
| Kconfig | `Kconfig` | Application-specific configuration options |

## Features

### Input/Output
- **Wheel Sensor:** GPIO interrupt-driven reed switch or Hall-effect sensor input
- **Buttons:** Two GPIO buttons for settings navigation (diameter adjustment)
- **Battery Monitoring:** ADC-based voltage reading with percentage calculation
- **Display:** SSD1306 OLED display support (I2C) - shows speed, distance, wheel size, battery
- **Console:** Serial output for debugging and data display
- **SD Card:** SPI-based SD card for logging trip data to `logs/app.log`

### Functionality
- **Speed Calculation:** km/h based on wheel circumference and revolution timing
- **Distance Tracking:** Total distance traveled in meters
- **Wheel Configuration:** Adjustable wheel diameter (10-100 cm) via buttons
- **Settings Mode:** Double-press button 1 to enter/exit; use buttons to adjust diameter
- **Persistent Storage:** Wheel diameter saved to flash using NVS (Non-Volatile Storage)
- **Debouncing:** 50ms debounce on all GPIO inputs

### Configuration Constants

| Constant | Default | Location | Description |
|----------|---------|----------|-------------|
| `DEBOUNCE_MS` | 50 | `main.c` | Debounce interval for all inputs |
| `REPORT_INTERVAL_MS` | 1000 | `main.c` | Speed update calculation interval |
| `MODE_SWITCH_DELAY_MS` | 500 | `main.c` | Double-press detection window |
| `DEFAULT_WHEEL_DIAMETER_CM` | 660 | `main.c` | Default wheel diameter (mm) |
| `MIN_WHEEL_DIAMETER_CM` | 10 | `main.c` | Minimum configurable diameter |
| `MAX_WHEEL_DIAMETER_CM` | 100 | `main.c` | Maximum configurable diameter |
| `BATTERY_SAMPLE_INTERVAL_S` | 60 | `main.c` | Battery voltage sample interval |

## Build & Run

### Prerequisites
- Zephyr RTOS workspace with `zephyr` and `hal_stm32` (or relevant modules)
- `west` build tool installed
- Python virtual environment (optional, referenced by `build.sh`)

### Building

```bash
# For Raspberry Pi Pico
west build -b rpi_pico /path/to/rpi_speedometer

# For native simulation (64-bit)
west build -b native_sim//64 /path/to/rpi_speedometer

# Using the build script
./build.sh rpi_pico
./build.sh native_sim//64
```

### Flashing
```bash
# Hold BOOTSEL while plugging in Pico, then:
west flash
```

### Simulation Testing
```bash
# Build with simulation support
west build -b native_sim//64 -p

# Run the application
./build/zephyr/zephyr.exe

# Attach to console (in another terminal)
alacritty -e screen /dev/pts/N  # Adjust pts number as needed

# Use simulation shell commands:
# - sim reed          : Toggle reed switch once
# - sim wheel N M     : Simulate N revolutions with M ms interval
# - sim button1       : Simulate button 1 press
# - sim button2       : Simulate button 2 press
# - sim battery MV    : Set battery voltage to MV millivolts
```

## Devicetree Configuration

### Raspberry Pi Pico (rpi_pico.overlay)
- **Wheel Sensor:** GPIO0 pin 8 (GPIO 16 on Pico) - ACTIVE_LOW with PULL_UP
- **Button 1:** GPIO0 pin 9 (GPIO 17 on Pico) - ACTIVE_LOW with PULL_UP
- **Button 2:** GPIO0 pin 10 (GPIO 18 on Pico) - ACTIVE_LOW with PULL_UP
- **Battery ADC:** ADC channel 0 (GPIO 26)
- **SD Card:** SPI1 with CS on GPIO 17
- **Flash Partition:** 16KB storage partition at offset 0x20000 for NVS

### Native Simulation (native_sim_64.overlay)
- Maps same GPIO and ADC nodes but disables display emulation
- Uses emulated GPIO and ADC for testing

## Agent Instructions

### Code Style & Conventions
- Follow **Zephyr coding style** (kernel-style Linux coding style)
- Use `snprintk()` for safe string formatting
- Use `ARG_UNUSED()` for unused function parameters
- Atomic operations for shared state between ISR and main thread
- `volatile` for variables shared between ISR and main loop
- Prefer `k_*` Zephyr APIs over standard library where appropriate

### Common Patterns
- **GPIO with Devicetree:** Use `GPIO_DT_SPEC_GET()` with `DT_PATH(zephyr_user)`
- **Interrupt Handling:** `gpio_pin_interrupt_configure_dt()`, `gpio_add_callback()`
- **ADC Reading:** `ADC_DT_SPEC_GET_BY_IDX()`, `adc_read()` with sequence struct
- **Settings:** Register handlers with `settings_register()`, load with `settings_load()`
- **Display:** Use `cfb_framebuffer_*` APIs when `CONFIG_DISPLAY` and `CONFIG_CHARACTER_FRAMEBUFFER` enabled
- **SD Card:** Use `fs_mount()`, `fs_open()`, `fs_write()`, `fs_sync()`

### Configuration Options (prj.conf)

Enabled by default:
- `CONFIG_GPIO` - GPIO driver
- `CONFIG_ADC` - ADC driver
- `CONFIG_PRINTK`, `CONFIG_CONSOLE`, `CONFIG_SERIAL` - Console output
- `CONFIG_DISPLAY`, `CONFIG_I2C`, `CONFIG_SPI`, `CONFIG_SSD1306` - Display support
- `CONFIG_SETTINGS`, `CONFIG_NVS`, `CONFIG_FLASH` - Persistent settings
- `CONFIG_SDHC`, `CONFIG_SDMMC_STACK`, `CONFIG_FAT_FILESYSTEM_ELM` - SD card support
- `CONFIG_LOG` - Logging subsystem

### Key Data Structures

```c
// Wheel sensor state (shared between ISR and main)
struct wheel_sensor_state {
    atomic_t revolution_count;      // Incremented in ISR
    volatile int64_t last_trigger_ms; // For debouncing
};

// Button state (shared between ISR and main)
struct button_state {
    volatile int64_t last_press_ms[2];  // Debounce tracking
    volatile bool in_settings_mode;
    volatile bool save_wheel_diameter_pending;
    int64_t previous_press_time[2];    // For double-press detection
};

// Wheel configuration
struct wheel_config {
    int diameter_cm;  // Wheel diameter in centimeters
};

// Runtime tracking state (main thread only)
struct runtime_state {
    uint32_t last_count;
    uint32_t total_distance_m;
    float last_speed_kmh;
    int battery_percentage;
    float battery_voltage;
    bool battery_valid;
};
```

### Development Workflow Notes

1. **Testing Changes:** Use `native_sim//64` board for rapid iteration
2. **Simulating Input:** Use `sim` shell commands to test without hardware
3. **Devicetree Changes:** Modify overlays, not application code, for pin changes
4. **Adding Features:**
   - New hardware: Add to devicetree overlay first
   - New settings: Register settings handler, add to storage partition
   - New display: Ensure `CONFIG_DISPLAY` and related options are enabled

### File Modification Guidance

| File | When to Modify | Typical Changes |
|------|----------------|-----------------|
| `src/main.c` | Core logic changes, new features | Add new sensors, modify calculations, add state |
| `boards/rpi_pico.overlay` | Hardware wiring changes | Change GPIO pins, add new devices |
| `boards/native_sim_64.overlay` | Simulation devicetree | Match hardware overlay for testing |
| `prj.conf` | Enable/disable subsystems | Add new Zephyr modules |
| `Kconfig` | New configuration options | Add user-configurable settings |
| `src/sim_shell.c` | New simulation commands | Add test commands for new features |

### Common Pitfalls
- **Thread Safety:** ISR modifies `revolution_count` atomically; main thread reads it
- **Debouncing:** All GPIO inputs use 50ms debounce - adjust if needed for your hardware
- **Display:** Check `APP_HAS_DISPLAY` preprocessor guard before using display APIs
- **SD Card:** Check if `log_file.mp != NULL` before writing; filesystem may fail to mount
- **Settings:** Storage partition must be defined in overlay for NVS to work

### Debugging Tips
- Use `printk()` for console output (appears via USB CDC-ACM on Pico)
- For simulation: connect to UART with `screen /dev/pts/N 115200`
- Check device readiness with `device_is_ready()` before use
- Use `west build -p` for pristine build if devicetree changes aren't applying

## Repository Structure

```
.
├── AGENTS.md              # This file
├── CMakeLists.txt         # CMake build configuration
├── Kconfig               # Application Kconfig options
├── LICENSE
├── README.md             # User-facing documentation
├── build.sh              # Convenience build script
├── prj.conf              # Zephyr project configuration
├── schema.md             # (Purpose unclear - may be legacy)
├── boards/
│   ├── rpi_pico.overlay  # Pico devicetree overlay
│   ├── native_sim_64.overlay  # Simulation devicetree
│   └── native_sim.conf   # Simulation Kconfig
└── src/
    ├── main.c            # Main application
    └── sim_shell.c       # Simulation shell commands
```
