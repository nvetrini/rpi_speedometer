/*
 * Simple cycling computer - wheel sensor input with wheel diameter setting
 *
 * Reads a reed switch / Hall-effect sensor via interrupt-driven GPIO.
 * Each time the wheel magnet passes the sensor, an interrupt fires,
 * a revolution counter is incremented (with debouncing), and the
 * main loop periodically derives speed/distance from the count.
 *
 * Button 1 (+1cm) and Button 2 (-1cm) allow setting wheel diameter.
 * Two consecutive presses of button 1 within 500ms enter/exit settings mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/util.h>

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

/* Shared between ISR context and main loop - keep it simple/atomic-ish */
static volatile uint32_t revolution_count;
static volatile int64_t last_trigger_ms;

/* Button handling state */
static volatile int64_t last_button1_press_ms = 0;
static volatile int64_t last_button2_press_ms = 0;
static volatile bool in_settings_mode = false;

/* Wheel diameter setting (in cm) */
static int wheel_diameter_cm = 660; // Default: 660mm = 66cm (common 700x23c tire)
#define MIN_WHEEL_DIAMETER_CM 10
#define MAX_WHEEL_DIAMETER_CM 100

/* Forward declarations */
static void update_wheel_circumference(void);
static void save_wheel_diameter_setting(void);

/* Ignore any edge that arrives within this window of the previous one.
 * Mechanical reed switches bounce; this also rejects electrical noise.
 * Tune this if you find missed or double-counted revolutions. */
#define DEBOUNCE_MS 50

/* Calculate wheel circumference from diameter (in meters) */
static float wheel_circumference_m;

/* How often the main loop reports speed/distance */
#define REPORT_INTERVAL_MS 1000

static void wheel_sensor_triggered(const struct device *dev,
			    struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();

	if ((now - last_trigger_ms) < DEBOUNCE_MS) {
		return;
	}
	last_trigger_ms = now;
	revolution_count++;
}

static void button1_pressed(const struct device *dev,
			   struct gpio_callback *cb,
			   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();

	// Debounce
	if ((now - last_button1_press_ms) < DEBOUNCE_MS) {
		return;
	}
	last_button1_press_ms = now;

	// Check for double-press to enter/exit settings mode
	static int64_t previous_press_time = 0;
	if ((now - previous_press_time) < 500) {
		// Double press detected - toggle settings mode
		in_settings_mode = !in_settings_mode;
		if (in_settings_mode) {
			printk("Entering settings mode. Current diameter: %d cm\n", wheel_diameter_cm);
		} else {
			printk("Exiting settings mode. Wheel diameter set to: %d cm\n", wheel_diameter_cm);
			// Save the setting
			save_wheel_diameter_setting();
		}
	} else {
		// Single press - adjust diameter if in settings mode
		if (in_settings_mode) {
			wheel_diameter_cm = MIN(wheel_diameter_cm + 1, MAX_WHEEL_DIAMETER_CM);
			printk("Wheel diameter: %d cm\n", wheel_diameter_cm);
			update_wheel_circumference();
		}
	}
	previous_press_time = now;
}

static void button2_pressed(const struct device *dev,
			   struct gpio_callback *cb,
			   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();

	// Debounce
	if ((now - last_button2_press_ms) < DEBOUNCE_MS) {
		return;
	}
	last_button2_press_ms = now;

	// Single press - adjust diameter if in settings mode
	if (in_settings_mode) {
		wheel_diameter_cm = MAX(wheel_diameter_cm - 1, MIN_WHEEL_DIAMETER_CM);
		printk("Wheel diameter: %d cm\n", wheel_diameter_cm);
		update_wheel_circumference();
	}
}

/* Helper function to update wheel circumference from diameter */
static void update_wheel_circumference(void)
{
	// Circumference = π * diameter, convert cm to meters
	wheel_circumference_m = 3.1415926535f * (wheel_diameter_cm / 100.0f);
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
			wheel_diameter_cm = val;
			update_wheel_circumference();
			printk("Loaded wheel diameter: %d cm\n", wheel_diameter_cm);
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
					   &wheel_diameter_cm, sizeof(int));
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
	update_wheel_circumference();

	printk("Wheel diameter: %d cm, circumference: %.3f m\n",
	       wheel_diameter_cm, (double)wheel_circumference_m);

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
				gpio_init_callback(&button1_cb_data, button1_pressed,
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
				gpio_init_callback(&button2_cb_data, button2_pressed,
						   BIT(button2.pin));
				gpio_add_callback(button2.port, &button2_cb_data);
			}
		}
	}

	printk("Wheel sensor ready, waiting for revolutions...\n");
	printk("Press button 1 twice quickly to enter settings mode\n");

	uint32_t last_count = 0;
	float total_distance_m = 0.0f;

	while (1) {
		k_msleep(REPORT_INTERVAL_MS);

		// Skip processing if in settings mode
		if (in_settings_mode) {
			continue;
		}

		/* Snapshot the counter - a single 32-bit read is fine here */
		uint32_t current_count = revolution_count;
		uint32_t delta = current_count - last_count;
		last_count = current_count;

		float distance_this_period_m = delta * wheel_circumference_m;
		total_distance_m += distance_this_period_m;

		/* speed (km/h) = distance (m) / time (s) * 3.6 */
		float speed_kmh = (distance_this_period_m /
				    (REPORT_INTERVAL_MS / 1000.0f)) * 3.6f;

		printk("revs=%u  speed=%d.%01u km/h  distance=%d.%02u m  diameter=%d cm\n",
		       current_count,
		       (int)speed_kmh,
		       (unsigned int)((speed_kmh - (int)speed_kmh) * 10),
		       (int)total_distance_m,
		       (unsigned int)((total_distance_m - (int)total_distance_m) * 100),
		       wheel_diameter_cm);
	}

	return 0;
}
