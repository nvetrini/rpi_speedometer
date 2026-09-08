/*
 * app.c - Main application with modular architecture
 *
 * Coordinates all modules to implement the wheel sensor speedometer.
 *
 * @implements REQ-SW-001, REQ-SW-002, REQ-SW-003, REQ-SW-004, REQ-SW-005, REQ-SW-006, REQ-SW-007, REQ-SW-008
 */

#include <app.h>
#include <wheel_sensor.h>
#include <button.h>
#include <battery.h>
#include <speed_calculator.h>
#include <display_output.h>
#include <console_output.h>
#include <storage.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_LEVEL_GLOBAL);

/* Global state instances */
static struct wheel_sensor_state wheel_sensor_state = {
	.revolution_count = ATOMIC_INIT(0),
	.last_trigger_ms = 0,
};

static struct button_state button_state = {
	.last_press_ms = {0, 0},
	.in_settings_mode = false,
	.save_wheel_diameter_pending = false,
	.previous_press_time = {0, 0},
	.settings_mode_changed = false,
	.diameter_changed = false,
	.diameter_value = DEFAULT_WHEEL_DIAMETER_CM,
};

static struct wheel_config wheel_config = {
	.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM,
};

static struct runtime_state runtime_state = {
	.last_count = 0U,
	.total_distance_m = 0U,
	.last_speed_kmh = 0.0f,
};

static struct battery_state battery_state = {
	.percentage = -1,
	.voltage = 0.0f,
	.valid = false,
};

static void app_state_work_handler(struct k_work *work);

K_WORK_DEFINE(app_state_work, app_state_work_handler);

/**
 * @brief Work handler for deferred state updates
 * @implements REQ-SW-003, REQ-SW-601
 * @param work Work item (unused)
 */
static void app_state_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (button_save_pending(&button_state)) {
		button_clear_save_pending(&button_state);
		storage_save_wheel_diameter(&wheel_config);
	}

	if (display_is_available()) {
		display_render(&button_state, &wheel_config, &runtime_state, &battery_state);
	}
}

/**
 * @brief Queue deferred state update
 * @implements REQ-SW-601
 */
static void queue_app_state_update(void)
{
	k_work_submit(&app_state_work);
}

/**
 * @brief Main application entry point
 * @implements REQ-SW-001, REQ-SW-002, REQ-SW-003, REQ-SW-004, REQ-SW-005, REQ-SW-006, REQ-SW-007, REQ-SW-008
 * @return Application exit code
 */
int app_run(void)
{
	int ret;

	/* Initialize storage/NVS first so settings can be loaded */
	ret = storage_init(&wheel_config);
	if (ret < 0) {
		LOG_ERR("Storage initialization failed: %d", -ret);
		return ret;
	}

	console_print_init(wheel_config.diameter_cm);

	/* Initialize display */
	ret = display_init();
	if (ret < 0) {
		LOG_WRN("Display initialization failed: %d, continuing without display", -ret);
	} else {
		queue_app_state_update();
	}

	/* Initialize SD card for logging */
	ret = storage_sd_init();
	if (ret < 0) {
		LOG_WRN("SD card initialization failed: %d, continuing without SD card", -ret);
	} else {
		ret = storage_log_open();
		if (ret < 0) {
			LOG_WRN("Failed to open log file: %d, continuing without SD card logging", -ret);
		} else {
			LOG_INF("SD card logging initialized");
		}
	}

	/* Initialize battery monitoring */
	ret = battery_init();
	if (ret < 0) {
		LOG_ERR("Battery initialization failed: %d", -ret);
		return ret;
	}

	/* Initialize wheel sensor */
	ret = wheel_sensor_init(&wheel_sensor_state);
	if (ret < 0) {
		LOG_ERR("Wheel sensor initialization failed: %d", -ret);
		return ret;
	}

	/* Initialize buttons */
	button_set_wheel_config(&wheel_config);
	ret = button_init(&button_state);
	if (ret < 0) {
		LOG_WRN("Button initialization failed: %d, continuing without buttons", -ret);
	}

	uint32_t battery_sample_counter = 0;

	while (1) {
		k_msleep(REPORT_INTERVAL_MS);

		/* Sample battery voltage periodically */
		battery_sample_counter++;
		if (battery_sample_counter >= BATTERY_SAMPLE_INTERVAL_S) {
			int battery_ret = battery_read(&battery_state);
			if (battery_ret < 0) {
				LOG_ERR("Battery read failed: %d", battery_ret);
			} else {
				console_print_battery(battery_state.percentage, battery_state.voltage);
			}
			battery_sample_counter = 0;
		}

		/* Handle deferred button press messages (from ISR) */
		if (button_state.settings_mode_changed) {
			button_state.settings_mode_changed = false;
			if (button_state.in_settings_mode) {
				LOG_INF("Entering settings mode. Current diameter: %d cm",
					button_state.diameter_value);
			} else {
				LOG_INF("Exiting settings mode. Wheel diameter set to: %d cm",
					button_state.diameter_value);
			}
		}
		if (button_state.diameter_changed) {
			button_state.diameter_changed = false;
			LOG_INF("Wheel diameter: %d cm", button_state.diameter_value);
		}

		/* If in settings mode, skip speed calculation but still update display */
		if (button_in_settings_mode(&button_state)) {
			queue_app_state_update();
			continue;
		}

		/* Get current revolution count */
		uint32_t current_count = wheel_sensor_get_count(&wheel_sensor_state);

		/* Calculate speed and distance */
		speed_calculator_update(
			current_count, runtime_state.last_count,
			wheel_config.diameter_cm, REPORT_INTERVAL_MS,
			&runtime_state.total_distance_m, &runtime_state.last_speed_kmh);
		runtime_state.last_count = current_count;

		/* Print status to console */
		console_print_status(
			current_count, runtime_state.last_speed_kmh,
			runtime_state.total_distance_m, wheel_config.diameter_cm);

		/* Log to SD card if available */
		if (storage_log_available()) {
			char log_msg[LOG_BUFFER_SIZE];
			snprintk(log_msg, sizeof(log_msg),
				"[%lld] revs=%u, speed=%.1f km/h, distance=%u m, diameter=%d cm\n",
				k_uptime_get(), current_count, (double)runtime_state.last_speed_kmh,
				runtime_state.total_distance_m, wheel_config.diameter_cm);
			storage_log_write(log_msg);
		}

		queue_app_state_update();
	}

	return 0;
}
