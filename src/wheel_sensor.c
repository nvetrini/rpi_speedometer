/*
 * wheel_sensor.c - Wheel sensor module
 *
 * Handles wheel revolution counting via GPIO interrupt with debouncing.
 */

#include "wheel_sensor.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Devicetree configuration */
#define USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec wheel_sensor =
	GPIO_DT_SPEC_GET(USER_NODE, wheel_sensor_gpios);

static struct wheel_sensor_state *sensor_state_ptr = NULL;
static struct gpio_callback wheel_sensor_cb_data;

int wheel_sensor_init(struct wheel_sensor_state *sensor_state)
{
	int ret;

	if (!device_is_ready(wheel_sensor.port)) {
		return -ENODEV;
	}

	sensor_state_ptr = sensor_state;

	sensor_state_ptr = sensor_state;

	/* Configure wheel sensor GPIO */
	ret = gpio_pin_configure_dt(&wheel_sensor, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	/* Configure interrupt */
	ret = gpio_pin_interrupt_configure_dt(&wheel_sensor,
				GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	/* Set up callback */
	gpio_init_callback(&wheel_sensor_cb_data, wheel_sensor_triggered,
			BIT(wheel_sensor.pin));
	gpio_add_callback(wheel_sensor.port, &wheel_sensor_cb_data);

	return 0;
}

uint32_t wheel_sensor_get_count(const struct wheel_sensor_state *sensor_state)
{
	return atomic_get(&sensor_state->revolution_count);
}

void wheel_sensor_triggered(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (sensor_state_ptr == NULL) {
		return;
	}

	int64_t now = k_uptime_get();

	/* Debounce check */
	if ((now - sensor_state_ptr->last_trigger_ms) < DEBOUNCE_MS) {
		return;
	}

	sensor_state_ptr->last_trigger_ms = now;
	atomic_inc(&sensor_state_ptr->revolution_count);
}
