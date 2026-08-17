/*
 * console_output.c - Console output module
 *
 * Handles all console/printk output for the application.
 */

#include "console_output.h"
#include <zephyr/sys/printk.h>

void console_print_status(uint32_t current_count, float speed_kmh,
		uint32_t total_distance_m, int wheel_diameter_cm)
{
	unsigned int speed_frac = (unsigned int)((speed_kmh - (int)speed_kmh) * 10.0f);
	if (speed_frac > 9U) {
		speed_frac = 9U;
	}

	printk("revs=%u  speed=%d.%01u km/h  distance=%u m  diameter=%d cm\n",
		current_count,
		(int)speed_kmh,
		speed_frac,
		total_distance_m,
		wheel_diameter_cm);
}

void console_print_settings_mode(bool in_settings_mode, int diameter_cm)
{
	if (in_settings_mode) {
		printk("Entering settings mode. Current diameter: %d cm\n", diameter_cm);
	} else {
		printk("Exiting settings mode. Wheel diameter set to: %d cm\n", diameter_cm);
	}
}

void console_print_diameter(int diameter_cm)
{
	printk("Wheel diameter: %d cm\n", diameter_cm);
}

void console_print_battery(int percentage, float voltage)
{
	printk("Battery: %d%% (%.2fV)\n", percentage, (double)voltage);
}

void console_print_init(int diameter_cm)
{
	printk("Wheel diameter: %d cm\n", diameter_cm);
	printk("Wheel sensor ready, waiting for revolutions...\n");
	printk("Press button 1 twice quickly to enter settings mode\n");
}

void console_print_error(const char *msg)
{
	printk("Error: %s\n", msg);
}

void console_print_warning(const char *msg)
{
	printk("Warning: %s\n", msg);
}
