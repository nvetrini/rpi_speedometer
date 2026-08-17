/*
 * battery.c - Battery monitoring module
 *
 * Handles ADC-based battery voltage monitoring and percentage calculation.
 */

#include "battery.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

/* Devicetree configuration */
#define USER_NODE DT_PATH(zephyr_user)
static const struct adc_dt_spec battery_adc =
	ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 0);

int battery_init(void)
{
	if (!device_is_ready(battery_adc.dev)) {
		return -ENODEV;
	}
	return 0;
}

int battery_read(struct battery_state *battery_state)
{
	if (!device_is_ready(battery_adc.dev)) {
		return -ENODEV;
	}

	int16_t adc_value;
	struct adc_sequence sequence = {
		.channels = BIT(battery_adc.channel_id),
		.buffer = &adc_value,
		.buffer_size = sizeof(adc_value),
		.resolution = 12,
	};

	int ret = adc_read(battery_adc.dev, &sequence);
	if (ret < 0) {
		printk("Battery ADC read error: %d\n", ret);
		return ret;
	}

	float battery_voltage = battery_adc_to_voltage(adc_value);
	int percentage = battery_voltage_to_percentage(battery_voltage);

	battery_state->voltage = battery_voltage;
	battery_state->percentage = percentage;
	battery_state->valid = true;

	return 0;
}

float battery_adc_to_voltage(int16_t adc_value)
{
	/* Convert ADC value to voltage (12-bit ADC, 3.3V reference) */
	float adc_voltage = (adc_value * 3.3f) / 4095.0f;

	/* Apply voltage divider ratio: V_battery = V_adc * (R1+R2)/R2 */
	return adc_voltage * VOLTAGE_DIVIDER_RATIO;
}

int battery_voltage_to_percentage(float voltage)
{
	/* Linear approximation for alkaline 2xAA */
	int percentage = (int)((voltage - BATTERY_MIN_V) /
			(BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f);
	return CLAMP(percentage, 0, 100);
}
