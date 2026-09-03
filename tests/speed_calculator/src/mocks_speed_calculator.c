/**
 * @file mocks_speed_calculator.c
 * @brief Mock implementations for speed calculator testing
 */

#include <zephyr/kernel.h>
#include <app.h>
#include <speed_calculator.h>

// FIXME: maybe use math.h in GNU mode to provide it
#ifndef M_PI
#define M_PI 3.1415926535f
#endif

/* Mock implementation of speed_calculator_update */
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

/* Mock implementation of speed_calculator_circumference_m */
float speed_calculator_circumference_m(int diameter_cm)
{
    return (float)(diameter_cm * 0.01f * M_PI);
}

/* Mock implementation of speed_calculator_speed_kmh */
float speed_calculator_speed_kmh(float distance_m, float time_s)
{
    if (time_s <= 0.0f) {
        return 0.0f;
    }
    return (distance_m / time_s) * 3.6f; /* Convert m/s to km/h */
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    /* No state to reset for speed calculator mocks */
}
