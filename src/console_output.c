/*
 * console_output.c - Console output module
 *
 * Handles all console/printk output for the application.
 *
 * @implements REQ-SW-701, REQ-SW-702, REQ-SW-703, REQ-SW-704, REQ-SW-705, REQ-SW-706, REQ-SW-707, REQ-SW-708
 */

#include <console_output.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(console);

/**
 * @brief Print status information to console
 * @implements REQ-SW-701, REQ-SW-702
 * @param current_count Current revolution count
 * @param speed_kmh Current speed in km/h
 * @param total_distance_m Total distance in meters
 * @param wheel_diameter_cm Wheel diameter in centimeters
 */
void console_print_status(uint32_t current_count, float speed_kmh,
		uint32_t total_distance_m, int wheel_diameter_cm)
{
	unsigned int speed_frac = (unsigned int)((speed_kmh - (int)speed_kmh) * 10.0f);
	if (speed_frac > 9U) {
		speed_frac = 9U;
	}

	LOG_DBG("revs=%u  speed=%d.%01u km/h  distance=%u m  diameter=%d cm",
		current_count,
		(int)speed_kmh,
		speed_frac,
		total_distance_m,
		wheel_diameter_cm);
}

/**
 * @brief Print settings mode information to console
 * @implements REQ-SW-706
 * @param in_settings_mode True if entering settings mode, false if exiting
 * @param diameter_cm Current wheel diameter in centimeters
 */
void console_print_settings_mode(bool in_settings_mode, int diameter_cm)
{
	if (in_settings_mode) {
		LOG_INF("Entering settings mode. Current diameter: %d cm", diameter_cm);
	} else {
		LOG_INF("Exiting settings mode. Wheel diameter set to: %d cm", diameter_cm);
	}
}

/**
 * @brief Print wheel diameter change to console
 * @implements REQ-SW-707
 * @param diameter_cm Current wheel diameter in centimeters
 */
void console_print_diameter(int diameter_cm)
{
	LOG_INF("Wheel diameter: %d cm", diameter_cm);
}

/**
 * @brief Print battery status to console
 * @implements REQ-SW-708
 * @param percentage Battery percentage
 * @param voltage Battery voltage in volts
 */
void console_print_battery(int percentage, float voltage)
{
	LOG_INF("Battery: %d%% (%.2fV)", percentage, (double)voltage);
}

/**
 * @brief Print initialization message to console
 * @implements REQ-SW-705
 * @param diameter_cm Current wheel diameter in centimeters
 */
void console_print_init(int diameter_cm)
{
	LOG_INF("Wheel diameter: %d cm", diameter_cm);
	LOG_INF("Wheel sensor ready, waiting for revolutions...");
	LOG_INF("Press button 1 twice quickly to enter settings mode");
}
