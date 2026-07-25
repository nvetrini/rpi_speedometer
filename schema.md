# Battery Power and Monitoring Schematic for RPi Pico

## Overview
This document describes how to power the Raspberry Pi Pico from a 3V battery and monitor its voltage using the built-in ADC.

---

## Connection Schematic

```
        3V Battery
       ┌───────────┐
 (+) ──┤   +       │──── VSYS (Pin 39 on Pico)
       │           │
       └─────┬─────┘
             │
             ├─[100kΩ]───┬─── GPIO26 (ADC0, Pin 31)
             │           │
            [200kΩ]      │
             │           │
             └─────┬─────┘
                   │
 (+) ── Battery (+) │ VSYS
 (-) ── Battery (-) └── GND (Pin 38 on Pico)
```

---

## Pin Connections

| Component | RPi Pico Pin | Physical Pin | Function |
|-----------|--------------|---------------|----------|
| Battery (+) | VSYS | 39 | Power input |
| Battery (-) | GND | 38 | Ground |
| Voltage divider output | GPIO26 | 31 | ADC0 input |

---

## Voltage Divider Details

- **R1 (upper resistor):** 100kΩ between battery (+) and ADC pin
- **R2 (lower resistor):** 200kΩ between ADC pin and GND
- **Ratio:** 1:2 (output = input × 200/(100+200) = input × 2/3)

### Voltage Calculation

| Battery Voltage | ADC Input Voltage |
|------------------|-------------------|
| 3.0V | 2.0V |
| 2.5V | 1.67V |
| 2.0V | 1.33V |
| 1.5V | 1.0V |

---

## Current Consumption

- **Voltage divider current:** V_battery / (R1 + R2) = 3V / 300kΩ = **10μA**
- **Impact:** Negligible on battery life

---

## Notes

1. **VSYS Pin:** When USB power is connected, VSYS is powered from the USB input. The battery can remain connected without issues.

2. **ADC Reference:** The RP2040 ADC uses a 3.3V reference voltage. The voltage divider ensures the input never exceeds this limit.

3. **ADC Pins Available on Pico:**
   - GPIO26 (ADC0)
   - GPIO27 (ADC1)
   - GPIO28 (ADC2)

4. **Alternative Resistor Values:** You can use other resistor combinations that maintain a similar ratio (e.g., 50kΩ + 100kΩ), but higher values reduce current draw and are preferred for battery-powered applications.

---

## Devicetree Configuration

See `boards/rpi_pico.overlay` for the ADC channel definition.

## Software Configuration

See `prj.conf` for required Kconfig options (CONFIG_ADC=y).
