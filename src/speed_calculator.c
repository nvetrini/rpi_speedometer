/*
 * speed_calculator.c - Speed and distance calculation module
 *
 * Handles all speed and distance calculations based on wheel revolutions.
 *
 * @implements REQ-SW-201, REQ-SW-202, REQ-SW-203, REQ-SW-204, REQ-SW-205, REQ-SW-206, REQ-SW-207, REQ-SW-208
 * @tests tests/speed_calculator/src/test_speed_calculator.c
 */

#include <speed_calculator.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

/**
 * @brief Calculate wheel circumference in meters
 * @implements REQ-SW-201
 * @param diameter_cm Wheel diameter in centimeters
 * @return Circumference in meters
 */
float speed_calculator_circumference_m(int diameter_cm)
{
	/* Circumference = PI * diameter (convert cm to m) */
	return M_PI * (diameter_cm / 100.0f);
}

/**
 * @brief Calculate distance traveled from revolutions
 * @implements REQ-SW-202
 * @param count Number of revolutions
 * @param circumference_m Wheel circumference in meters
 * @return Distance in meters
 */
float speed_calculator_distance_m(uint32_t count, float circumference_m)
{
	return count * circumference_m;
}

/**
 * @brief Calculate speed from distance and time
 * @implements REQ-SW-203, REQ-SW-206
 * @param distance_m Distance in meters
 * @param time_s Time in seconds
 * @return Speed in km/h
 */
float speed_calculator_speed_kmh(float distance_m, float time_s)
{
	/* speed (km/h) = distance (m) / time (s) * 3.6 */
	if (time_s <= 0.0f) {
		return 0.0f;
	}
	return (distance_m / time_s) * 3.6f;
}

/**
 * @brief Calculate speed and distance from wheel revolutions
 * @implements REQ-SW-204, REQ-SW-205, REQ-SW-207, REQ-SW-208
 * @param current_count Current revolution count
 * @param last_count Previous revolution count
 * @param wheel_diameter_cm Wheel diameter in centimeters
 * @param interval_ms Time interval in milliseconds
 * @param total_distance_m Pointer to total distance in meters (in/out)
 * @param speed_kmh Pointer to speed in km/h (out)
 */
void speed_calculator_update(uint32_t current_count, uint32_t last_count,
		int wheel_diameter_cm, uint32_t interval_ms,
		uint32_t *total_distance_m, float *speed_kmh)
{
	uint32_t delta = current_count - last_count;
	float circumference_m = speed_calculator_circumference_m(wheel_diameter_cm);
	
	/* Distance traveled this period */
	float distance_this_period_m = delta * circumference_m;
	
	/* Total distance
	 * FIXME: Using float for intermediate calculation loses precision for large
	 * counts (> 2^24 ~16.7M revolutions). For a 66cm wheel, this corresponds to
	 * ~52km of travel. Consider using uint64_t for intermediate if higher precision
	 * is needed for long-distance tracking. */
	*total_distance_m = (uint32_t)(current_count * circumference_m);
	
	/* Speed: distance / time * 3.6 */
	float time_s = interval_ms / 1000.0f;
	*speed_kmh = speed_calculator_speed_kmh(distance_this_period_m, time_s);
}
