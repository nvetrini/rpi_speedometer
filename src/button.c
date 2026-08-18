/*
 * button.c - Button input module
 *
 * Handles button input with debouncing and settings mode management.
 * Button 1: Double-press toggles settings mode, single press increments diameter
 * Button 2: Single press decrements diameter in settings mode
 */

#include "button.h"
#include "app.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/* Devicetree configuration */
#define USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec button1 =
	GPIO_DT_SPEC_GET(USER_NODE, button1_gpios);
static const struct gpio_dt_spec button2 =
	GPIO_DT_SPEC_GET(USER_NODE, button2_gpios);

static struct button_state *button_state_ptr = NULL;
static struct wheel_config *wheel_config_ptr = NULL;

/* Button callback data structure with user data */
struct button_cb_data {
	struct gpio_callback callback;
	int button_idx;
};
static struct button_cb_data button_cb_data[2];

/* Internal processing function */
static void process_button_press(int button_idx)
{
	int64_t now = k_uptime_get();

	if (button_state_ptr == NULL || wheel_config_ptr == NULL) {
		return;
	}

	/* Debounce check */
	if ((now - button_state_ptr->last_press_ms[button_idx]) < DEBOUNCE_MS) {
		return;
	}
	button_state_ptr->last_press_ms[button_idx] = now;

	if (button_idx == 0) {
		/* Button 1: double press toggles settings mode; single press increments diameter */
		if ((now - button_state_ptr->previous_press_time[button_idx]) < MODE_SWITCH_DELAY_MS) {
			button_state_ptr->in_settings_mode = !button_state_ptr->in_settings_mode;
			if (button_state_ptr->in_settings_mode) {
				printk("Entering settings mode. Current diameter: %d cm\n",
					wheel_config_ptr->diameter_cm);
			} else {
				printk("Exiting settings mode. Wheel diameter set to: %d cm\n",
					wheel_config_ptr->diameter_cm);
				button_state_ptr->save_wheel_diameter_pending = true;
			}
		} else {
			if (button_state_ptr->in_settings_mode) {
				wheel_config_ptr->diameter_cm =
					MIN(wheel_config_ptr->diameter_cm + 1, MAX_WHEEL_DIAMETER_CM);
				printk("Wheel diameter: %d cm\n", wheel_config_ptr->diameter_cm);
			}
		}
		button_state_ptr->previous_press_time[button_idx] = now;
	} else {
		/* Button 2: single press decrements diameter in settings mode */
		if (button_state_ptr->in_settings_mode) {
			wheel_config_ptr->diameter_cm =
				MAX(wheel_config_ptr->diameter_cm - 1, MIN_WHEEL_DIAMETER_CM);
			printk("Wheel diameter: %d cm\n", wheel_config_ptr->diameter_cm);
		}
	}
}

/* Unified callback for all buttons */
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct button_cb_data *data = CONTAINER_OF(cb, struct button_cb_data, callback);
	process_button_press(data->button_idx);
}

int button_init(struct button_state *button_state)
{
	int ret;

	button_state_ptr = button_state;

	/* Initialize button state */
	button_state->last_press_ms[0] = 0;
	button_state->last_press_ms[1] = 0;
	button_state->in_settings_mode = false;
	button_state->save_wheel_diameter_pending = false;
	button_state->previous_press_time[0] = 0;
	button_state->previous_press_time[1] = 0;

	if (!device_is_ready(button1.port) || !device_is_ready(button2.port)) {
		return -ENODEV;
	}

	/* Configure button 1 */
	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	button_cb_data[0].button_idx = 0;
	gpio_init_callback(&button_cb_data[0].callback, button_pressed, BIT(button1.pin));
	gpio_add_callback(button1.port, &button_cb_data[0].callback);

	/* Configure button 2 */
	ret = gpio_pin_configure_dt(&button2, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	button_cb_data[1].button_idx = 1;
	gpio_init_callback(&button_cb_data[1].callback, button_pressed, BIT(button2.pin));
	gpio_add_callback(button2.port, &button_cb_data[1].callback);

	return 0;
}

void button_set_wheel_config(struct wheel_config *wheel_config)
{
	wheel_config_ptr = wheel_config;
}
