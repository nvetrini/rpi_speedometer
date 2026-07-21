/*
 * Simple cycling computer - wheel sensor input
 *
 * Reads a reed switch / Hall-effect sensor via interrupt-driven GPIO.
 * Each time the wheel magnet passes the sensor, an interrupt fires,
 * a revolution counter is incremented (with debouncing), and the
 * main loop periodically derives speed/distance from the count.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Pulls the gpio spec (port/pin/flags) straight from the devicetree
 * overlay's "zephyr,user" node - no custom binding needed. */
static const struct gpio_dt_spec wheel_sensor =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wheel_sensor_gpios);

static struct gpio_callback wheel_sensor_cb_data;

/* Shared between ISR context and main loop - keep it simple/atomic-ish */
static volatile uint32_t revolution_count;
static volatile int64_t last_trigger_ms;

/* Ignore any edge that arrives within this window of the previous one.
 * Mechanical reed switches bounce; this also rejects electrical noise.
 * Tune this if you find missed or double-counted revolutions. */
#define DEBOUNCE_MS 50

/* Adjust to your wheel's rolling circumference in meters.
 * 2.105 m is roughly a 700x23c road tire. */
#define WHEEL_CIRCUMFERENCE_M 2.105

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

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&wheel_sensor)) {
		printk("Error: wheel sensor GPIO device not ready\n");
		return 0;
	}

	/* Flags (input, pull-up, active-low) come from the devicetree
	 * overlay, so we only need to request "input" here. */
	ret = gpio_pin_configure_dt(&wheel_sensor, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor pin\n", ret);
		return 0;
	}

	/* Fire when the pin transitions to its active state, i.e. when
	 * the magnet closes the reed switch / trips the Hall sensor. */
	ret = gpio_pin_interrupt_configure_dt(&wheel_sensor,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor interrupt\n", ret);
		return 0;
	}

	gpio_init_callback(&wheel_sensor_cb_data, wheel_sensor_triggered,
			   BIT(wheel_sensor.pin));
	gpio_add_callback(wheel_sensor.port, &wheel_sensor_cb_data);

	printk("Wheel sensor ready, waiting for revolutions...\n");

	uint32_t last_count = 0;
	float total_distance_m = 0.0f;

	while (1) {
		k_msleep(REPORT_INTERVAL_MS);

		/* Snapshot the counter - a single 32-bit read is fine here */
		uint32_t current_count = revolution_count;
		uint32_t delta = current_count - last_count;
		last_count = current_count;

		float distance_this_period_m = delta * WHEEL_CIRCUMFERENCE_M;
		total_distance_m += distance_this_period_m;

		/* speed (km/h) = distance (m) / time (s) * 3.6 */
		float speed_kmh = (distance_this_period_m /
				    (REPORT_INTERVAL_MS / 1000.0f)) * 3.6f;

		printk("revs=%u  speed=%d.%01u km/h  distance=%d.%02u m\n",
		       current_count,
		       (int)speed_kmh,
		       (unsigned int)((speed_kmh - (int)speed_kmh) * 10),
		       (int)total_distance_m,
		       (unsigned int)((total_distance_m - (int)total_distance_m) * 100));
	}

	return 0;
}
