/*
 * Simple cycling computer - wheel sensor input with wheel diameter setting
 *
 * Reads a reed switch / Hall-effect sensor via interrupt-driven GPIO.
 * Each time the wheel magnet passes the sensor, an interrupt fires,
 * a revolution counter is incremented (with debouncing), and the
 * main loop periodically derives speed/distance from the count.
 *
 * Button 1 (+1cm) and Button 2 (-1cm) allow setting wheel diameter.
 * Two consecutive presses of button 1 within MODE_SWITCH_DELAY_MSms enter/exit settings mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/util.h>

/* Constants */
#define MIN_WHEEL_DIAMETER_CM 10
#define MAX_WHEEL_DIAMETER_CM 100
#define DEBOUNCE_MS 50
#define REPORT_INTERVAL_MS 1000
#define MODE_SWITCH_DELAY_MS 500

/* Wheel configuration */
#define DEFAULT_WHEEL_DIAMETER_CM 660

/*
 * Application state structures
 */

/* Wheel sensor state - shared between ISR and main thread */
struct wheel_sensor_state {
	atomic_t revolution_count;
	volatile int64_t last_trigger_ms;
};

/* Button state - shared between ISR and main thread */
struct button_state {
	volatile int64_t last_press_ms[2];
	volatile bool in_settings_mode;
	int64_t previous_press_time[2];
};

/* Wheel configuration */
struct wheel_config {
	int diameter_cm;
};

/* Runtime tracking state (main thread only) */
struct runtime_state {
	uint32_t last_count;
	float total_distance_m;
};

/* Pulls the gpio spec (port/pin/flags) straight from the devicetree
 * overlay's "zephyr,user" node - no custom binding needed. */
static const struct gpio_dt_spec wheel_sensor =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wheel_sensor_gpios);
static const struct gpio_dt_spec button1 =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button1_gpios);
static const struct gpio_dt_spec button2 =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button2_gpios);

static struct gpio_callback wheel_sensor_cb_data;
static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;

/* Global state instances */
static struct wheel_sensor_state wheel_sensor_state = {
	.revolution_count = ATOMIC_INIT(0),
	.last_trigger_ms = 0,
};

static struct button_state button_state = {
	.last_press_ms = {0, 0},
	.in_settings_mode = false,
	.previous_press_time = {0, 0},
};

static struct wheel_config wheel_config = {
	.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM,
};

static struct runtime_state runtime_state = {
	.last_count = 0,
	.total_distance_m = 0.0f,
};

/* Forward declarations */
static void save_wheel_diameter_setting(void);

static void wheel_sensor_triggered(const struct device *dev,
			    struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();

	if ((now - wheel_sensor_state.last_trigger_ms) < DEBOUNCE_MS) {
		return;
	}
	wheel_sensor_state.last_trigger_ms = now;
	atomic_inc(&wheel_sensor_state.revolution_count);
}

static void button_pressed(const struct device *dev,
		   struct gpio_callback *cb,
		   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	int64_t now = k_uptime_get();

	int button_idx = -1;
	if (pins & BIT(button1.pin)) {
		button_idx = 0;
	} else if (pins & BIT(button2.pin)) {
		button_idx = 1;
	}
	if (button_idx < 0) {
		return;
	}

	// Debounce
	if ((now - button_state.last_press_ms[button_idx]) < DEBOUNCE_MS) {
		return;
	}
	button_state.last_press_ms[button_idx] = now;

	if (button_idx == 0) {
		// Button 1: Check for double-press to enter/exit settings mode
		if ((now - button_state.previous_press_time[button_idx]) < MODE_SWITCH_DELAY_MS) {
			button_state.in_settings_mode = !button_state.in_settings_mode;
			if (button_state.in_settings_mode) {
				printk("Entering settings mode. Current diameter: %d cm\n", wheel_config.diameter_cm);
			} else {
				printk("Exiting settings mode. Wheel diameter set to: %d cm\n", wheel_config.diameter_cm);
				// Save the setting
				save_wheel_diameter_setting();
			}
		} else {
			// Single press - adjust diameter if in settings mode
			if (button_state.in_settings_mode) {
				wheel_config.diameter_cm = MIN(wheel_config.diameter_cm + 1, MAX_WHEEL_DIAMETER_CM);
				printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);
			}
		}
		button_state.previous_press_time[button_idx] = now;
	} else {
		// Button 2: Single press - adjust diameter if in settings mode
		if (button_state.in_settings_mode) {
			wheel_config.diameter_cm = MAX(wheel_config.diameter_cm - 1, MIN_WHEEL_DIAMETER_CM);
			printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);
		}
	}
}

/* Settings handler for wheel diameter */
static int settings_wheel_diameter_handler(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	int val;

	if (strncmp(key, "wheel_diameter", len) == 0 && len == strlen("wheel_diameter")) {
		if (len != sizeof(int)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &val, sizeof(int));
		if (rc < 0) {
			return rc;
		}

		if (val >= MIN_WHEEL_DIAMETER_CM && val <= MAX_WHEEL_DIAMETER_CM) {
			wheel_config.diameter_cm = val;
			printk("Loaded wheel diameter: %d cm\n", wheel_config.diameter_cm);
		} else {
			printk("Invalid wheel diameter value: %d cm\n", val);
		}

		return 0;
	}

	return -ENOENT;
}

static struct settings_handler wheel_diameter_handler = {
	.name = "wheel_diameter",
	.h_set = settings_wheel_diameter_handler,
};

/* Save wheel diameter setting */
static void save_wheel_diameter_setting(void)
{
	int rc = settings_save_one("wheel_diameter/wheel_diameter",
					   &wheel_config.diameter_cm, sizeof(int));
	if (rc < 0) {
		printk("Failed to save wheel diameter setting: %d\n", rc);
	}
}

/* Load wheel diameter setting */
static void load_wheel_diameter_setting(void)
{
	int rc = settings_load_subtree("wheel_diameter");
	if (rc < 0 && rc != -ENOENT) {
		printk("Failed to load wheel diameter setting: %d\n", rc);
	}
}

int main(void)
{
	int ret;

	// Initialize settings subsystem
	settings_subsys_init();
	settings_register(&wheel_diameter_handler);
	settings_load();

	// Load wheel diameter setting
	load_wheel_diameter_setting();

	printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);

	if (!gpio_is_ready_dt(&wheel_sensor)) {
		printk("Error: wheel sensor GPIO device not ready\n");
		return 0;
	}

	/* Configure wheel sensor */
	ret = gpio_pin_configure_dt(&wheel_sensor, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor pin\n", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&wheel_sensor,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor interrupt\n", ret);
		return 0;
	}

	gpio_init_callback(&wheel_sensor_cb_data, wheel_sensor_triggered,
			   BIT(wheel_sensor.pin));
	gpio_add_callback(wheel_sensor.port, &wheel_sensor_cb_data);

	/* Configure buttons */
	if (!gpio_is_ready_dt(&button1) || !gpio_is_ready_dt(&button2)) {
		printk("Error: button GPIO devices not ready\n");
		// Continue without buttons
	} else {
		ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d configuring button1 pin\n", ret);
		} else {
			ret = gpio_pin_interrupt_configure_dt(&button1,
						      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret != 0) {
				printk("Error %d configuring button1 interrupt\n", ret);
			} else {
				gpio_init_callback(&button1_cb_data, button_pressed,
						   BIT(button1.pin));
				gpio_add_callback(button1.port, &button1_cb_data);
			}
		}

		ret = gpio_pin_configure_dt(&button2, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d configuring button2 pin\n", ret);
		} else {
			ret = gpio_pin_interrupt_configure_dt(&button2,
						      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret != 0) {
				printk("Error %d configuring button2 interrupt\n", ret);
			} else {
				gpio_init_callback(&button2_cb_data, button_pressed,
						   BIT(button2.pin));
				gpio_add_callback(button2.port, &button2_cb_data);
			}
		}
	}

	printk("Wheel sensor ready, waiting for revolutions...\n");
	printk("Press button 1 twice quickly to enter settings mode\n");

	/* Initialize runtime state - already done at file scope */

	while (1) {
		k_msleep(REPORT_INTERVAL_MS);

		// Skip processing if in settings mode
		if (button_state.in_settings_mode) {
			continue;
		}

		/* Snapshot the counter atomically */
		uint32_t current_count = atomic_get(&wheel_sensor_state.revolution_count);
		uint32_t delta = current_count - runtime_state.last_count;
		runtime_state.last_count = current_count;

		/* Circumference = π * diameter (convert cm to m) */
		float circumference_m = 3.1415926535f * (wheel_config.diameter_cm / 100.0f);
		float distance_this_period_m = delta * circumference_m;
		runtime_state.total_distance_m += distance_this_period_m;

		/* speed (km/h) = distance (m) / time (s) * 3.6 */
		float speed_kmh = (distance_this_period_m /
				    (REPORT_INTERVAL_MS / 1000.0f)) * 3.6f;

		printk("revs=%u  speed=%d.%01u km/h  distance=%d.%02u m  diameter=%d cm\n",
		       current_count,
		       (int)speed_kmh,
		       (unsigned int)((speed_kmh - (int)speed_kmh) * 10),
		       (int)runtime_state.total_distance_m,
		       (unsigned int)((runtime_state.total_distance_m - (int)runtime_state.total_distance_m) * 100),
		       wheel_config.diameter_cm);
	}

	return 0;
}
