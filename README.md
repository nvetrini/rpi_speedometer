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
