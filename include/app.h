/*
 * app.h - Main application header for wheel sensor speedometer
 *
 * Defines shared data structures, constants, and constants.
 * All modules should include this header for common types.
 *
 * Module-specific interfaces are in their respective header files:
 * - wheel_sensor.h
 * - button.h
 * - battery.h
 * - speed_calculator.h
 * - display_output.h
 * - console_output.h
 * - storage.h
 */

#ifndef APP_H
#define APP_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <stdbool.h>
#include <stdint.h>

/* =========================================================================
 * CONSTANTS
 * ========================================================================= */

/* Wheel configuration limits */
#define MIN_WHEEL_DIAMETER_CM    10
#define MAX_WHEEL_DIAMETER_CM    100
#define DEFAULT_WHEEL_DIAMETER_CM 660

/* Timing constants */
#define DEBOUNCE_MS              50
#define REPORT_INTERVAL_MS       1000
#define MODE_SWITCH_DELAY_MS     500
#define BATTERY_SAMPLE_INTERVAL_S 60

/* Battery monitoring constants */
#define BATTERY_MIN_V            2.0f   /* Voltage at 0% (2x AA depleted) */
#define BATTERY_MAX_V            3.0f   /* Voltage at 100% (2x AA fresh) */
#define VOLTAGE_DIVIDER_RATIO    (3.0f / 2.0f)  /* V_battery = V_adc * (R1+R2)/R2 */

/* Buffer sizes */
#define LOG_BUFFER_SIZE          256
#define DISPLAY_LINE_SIZE        32

/* =========================================================================
 * DATA STRUCTURES
 * ========================================================================= */

/* Wheel sensor state - shared between ISR and main thread */
struct wheel_sensor_state {
	atomic_t revolution_count;
	volatile int64_t last_trigger_ms;
};

/* Button state - shared between ISR and main thread */
struct button_state {
	volatile int64_t last_press_ms[2];
	volatile bool in_settings_mode;
	volatile bool save_wheel_diameter_pending;
	int64_t previous_press_time[2];
};

/* Wheel configuration */
struct wheel_config {
	int diameter_cm;
};

/* Battery state */
struct battery_state {
	int percentage;
	float voltage;
	bool valid;
};

/* Runtime tracking state (main thread only) */
struct runtime_state {
	uint32_t last_count;
	uint32_t total_distance_m;
	float last_speed_kmh;
};

/* =========================================================================
 * INLINE ACCESSORS
 * ========================================================================= */

/**
 * @brief Check if in settings mode
 * @param button_state Pointer to button state structure
 * @return true if in settings mode
 */
static inline bool button_in_settings_mode(const struct button_state *button_state)
{
	return button_state->in_settings_mode;
}

/**
 * @brief Check if wheel diameter needs saving
 * @param button_state Pointer to button state structure
 * @return true if pending save
 */
static inline bool button_save_pending(const struct button_state *button_state)
{
	return button_state->save_wheel_diameter_pending;
}

/**
 * @brief Clear save pending flag
 * @param button_state Pointer to button state structure
 */
static inline void button_clear_save_pending(struct button_state *button_state)
{
	button_state->save_wheel_diameter_pending = false;
}

#endif /* APP_H */
