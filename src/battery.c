/*
 * battery.c - Battery monitoring module
 *
 * Handles ADC-based battery voltage monitoring and percentage calculation.
 *
 * @implements REQ-HW-040, REQ-HW-041, REQ-HW-042, REQ-SW-401, REQ-SW-402, REQ-SW-403, REQ-SW-404, REQ-SW-405, REQ-SW-406, REQ-SW-407, REQ-SW-408, REQ-SW-409, REQ-SW-410
 * @tests tests/battery/src/test_battery.c
 */

#include <battery.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery);

/* Devicetree configuration */
#define USER_NODE DT_PATH(zephyr_user)
static const struct adc_dt_spec battery_adc =
	ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 0);

/**
 * @brief Initialize battery ADC
 * @implements REQ-SW-401
 * @return 0 on success, negative errno on failure
 */
int battery_init(void)
{
	if (!device_is_ready(battery_adc.dev)) {
		return -ENODEV;
	}
	return 0;
}

/**
 * @brief Read battery voltage and update state
 *
 * Battery must be already initialized via a successful call to
 * battery_init() prior to calling this function.
 *
 * @implements REQ-SW-401, REQ-SW-402, REQ-SW-410
 * @param battery_state Pointer to battery state structure to update
 * @return 0 on success, negative errno on failure
 */
int battery_read(struct battery_state *battery_state)
{
	int16_t adc_value;
	struct adc_sequence sequence = {
		.channels = BIT(battery_adc.channel_id),
		.buffer = &adc_value,
		.buffer_size = sizeof(adc_value),
		.resolution = 12,
	};

	int ret = adc_read(battery_adc.dev, &sequence);
	if (ret < 0) {
		LOG_ERR("Battery ADC read error: %d", ret);
		return ret;
	}

	float battery_voltage = battery_adc_to_voltage(adc_value);
	int percentage = battery_voltage_to_percentage(battery_voltage);

	battery_state->voltage = battery_voltage;
	battery_state->percentage = percentage;
	battery_state->valid = true;

	return 0;
}

/**
 * @brief Convert ADC value to battery voltage
 * @implements REQ-SW-403, REQ-SW-404, REQ-SW-405
 * @param adc_value Raw ADC value (12-bit)
 * @return Battery voltage in volts
 */
float battery_adc_to_voltage(int16_t adc_value)
{
	/* Convert ADC value to voltage (12-bit ADC, 3.3V reference)
	 * NOTE: The 3.3V reference is hardcoded. The actual RP2040 ADC reference
	 * can vary by +/- 1-2% which will cause a systematic offset in battery
	 * voltage readings. For more accurate readings, consider reading the
	 * actual Vref from ADC calibration data. */
	float adc_voltage = (adc_value * 3.3f) / 4095.0f;

	/* Apply voltage divider ratio: V_battery = V_adc * (R1+R2)/R2 */
	return adc_voltage * VOLTAGE_DIVIDER_RATIO;
}

/**
 * @brief Convert battery voltage to percentage
 * @implements REQ-SW-406, REQ-SW-407, REQ-SW-408, REQ-SW-409
 * @param voltage Battery voltage in volts
 * @return Percentage (0-100)
 */
int battery_voltage_to_percentage(float voltage)
{
	/* Linear approximation for alkaline 2xAA */
	int percentage = (int)((voltage - BATTERY_MIN_V) /
			(BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f);
	return CLAMP(percentage, 0, 100);
}
