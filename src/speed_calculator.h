/*
 * speed_calculator.h - Speed and distance calculation module interface
 *
 * Handles all speed and distance calculations based on wheel revolutions.
 */

#ifndef SPEED_CALCULATOR_H
#define SPEED_CALCULATOR_H

#include <stdint.h>

/**
 * @brief Calculate speed and distance from wheel revolutions
 * @param current_count Current revolution count
 * @param last_count Previous revolution count
 * @param wheel_diameter_cm Wheel diameter in centimeters
 * @param interval_ms Time interval in milliseconds
 * @param total_distance_m Pointer to total distance in meters (in/out)
 * @param speed_kmh Pointer to speed in km/h (out)
 */
void speed_calculator_update(uint32_t current_count, uint32_t last_count,
		int wheel_diameter_cm, uint32_t interval_ms,
		uint32_t *total_distance_m, float *speed_kmh);

/**
 * @brief Calculate wheel circumference in meters
 * @param diameter_cm Wheel diameter in centimeters
 * @return Circumference in meters
 */
float speed_calculator_circumference_m(int diameter_cm);

/**
 * @brief Calculate distance traveled from revolutions
 * @param count Number of revolutions
 * @param circumference_m Wheel circumference in meters
 * @return Distance in meters
 */
float speed_calculator_distance_m(uint32_t count, float circumference_m);

/**
 * @brief Calculate speed from distance and time
 * @param distance_m Distance in meters
 * @param time_s Time in seconds
 * @return Speed in km/h
 */
float speed_calculator_speed_kmh(float distance_m, float time_s);

#endif /* SPEED_CALCULATOR_H */
