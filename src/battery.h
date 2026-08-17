/*
 * battery.h - Battery monitoring module interface
 *
 * Handles ADC-based battery voltage monitoring and percentage calculation.
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "app.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <stdbool.h>

/**
 * @brief Initialize battery ADC
 * @return 0 on success, negative errno on failure
 */
int battery_init(void);

/**
 * @brief Read battery voltage and update state
 * @param battery_state Pointer to battery state structure to update
 * @return 0 on success, negative errno on failure
 */
int battery_read(struct battery_state *battery_state);

/**
 * @brief Convert ADC value to battery voltage
 * @param adc_value Raw ADC value (12-bit)
 * @return Battery voltage in volts
 */
float battery_adc_to_voltage(int16_t adc_value);

/**
 * @brief Convert battery voltage to percentage
 * @param voltage Battery voltage in volts
 * @return Percentage (0-100)
 */
int battery_voltage_to_percentage(float voltage);

#endif /* BATTERY_H */
