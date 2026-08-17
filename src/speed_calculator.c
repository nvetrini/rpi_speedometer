/*
 * speed_calculator.c - Speed and distance calculation module
 *
 * Handles all speed and distance calculations based on wheel revolutions.
 */

#include "speed_calculator.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535f
#endif

float speed_calculator_circumference_m(int diameter_cm)
{
	/* Circumference = PI * diameter (convert cm to m) */
	return M_PI * (diameter_cm / 100.0f);
}

float speed_calculator_distance_m(uint32_t count, float circumference_m)
{
	return count * circumference_m;
}

float speed_calculator_speed_kmh(float distance_m, float time_s)
{
	/* speed (km/h) = distance (m) / time (s) * 3.6 */
	if (time_s <= 0.0f) {
		return 0.0f;
	}
	return (distance_m / time_s) * 3.6f;
}

void speed_calculator_update(uint32_t current_count, uint32_t last_count,
		int wheel_diameter_cm, uint32_t interval_ms,
		uint32_t *total_distance_m, float *speed_kmh)
{
	uint32_t delta = current_count - last_count;
	float circumference_m = speed_calculator_circumference_m(wheel_diameter_cm);
	
	/* Distance traveled this period */
	float distance_this_period_m = delta * circumference_m;
	
	/* Total distance */
	*total_distance_m = (uint32_t)(current_count * circumference_m);
	
	/* Speed: distance / time * 3.6 */
	float time_s = interval_ms / 1000.0f;
	*speed_kmh = speed_calculator_speed_kmh(distance_this_period_m, time_s);
}
