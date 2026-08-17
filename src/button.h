/*
 * button.h - Button input module interface
 *
 * Handles button input with debouncing and settings mode management.
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "app.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Initialize button GPIOs and interrupts
 * @param button_state Pointer to button state structure
 * @return 0 on success, negative errno on failure
 */
int button_init(struct button_state *button_state);

/**
 * @brief Set wheel config pointer for button callbacks
 * @param wheel_config Pointer to wheel configuration
 */
void button_set_wheel_config(struct wheel_config *wheel_config);

/**
 * @brief Get button count
 * @return Number of buttons (currently 2)
 */
static inline int button_count(void)
{
	return 2;
}

#endif /* BUTTON_H */
