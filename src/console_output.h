/*
 * console_output.h - Console output module interface
 *
 * Handles all console/printk output for the application.
 */

#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Print status to console
 * @param current_count Current revolution count
 * @param speed_kmh Current speed in km/h
 * @param total_distance_m Total distance in meters
 * @param wheel_diameter_cm Wheel diameter in centimeters
 */
void console_print_status(uint32_t current_count, float speed_kmh,
		uint32_t total_distance_m, int wheel_diameter_cm);

/**
 * @brief Print settings mode status
 * @param in_settings_mode true if entering settings mode
 * @param diameter_cm Current wheel diameter
 */
void console_print_settings_mode(bool in_settings_mode, int diameter_cm);

/**
 * @brief Print wheel diameter update
 * @param diameter_cm New wheel diameter
 */
void console_print_diameter(int diameter_cm);

/**
 * @brief Print battery status
 * @param percentage Battery percentage
 * @param voltage Battery voltage
 */
void console_print_battery(int percentage, float voltage);

/**
 * @brief Print initialization message
 * @param diameter_cm Wheel diameter
 */
void console_print_init(int diameter_cm);

/**
 * @brief Print error message
 * @param msg Error message
 */
void console_print_error(const char *msg);

/**
 * @brief Print warning message
 * @param msg Warning message
 */
void console_print_warning(const char *msg);

#endif /* CONSOLE_OUTPUT_H */
