/*
 * wheel_sensor.h - Wheel sensor module interface
 *
 * Handles wheel revolution counting via GPIO interrupt.
 */

#ifndef WHEEL_SENSOR_H
#define WHEEL_SENSOR_H

#include "app.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Initialize wheel sensor GPIO and interrupt
 * @param sensor_state Pointer to wheel sensor state structure
 * @return 0 on success, negative errno on failure
 */
int wheel_sensor_init(struct wheel_sensor_state *sensor_state);

/**
 * @brief Get current revolution count (atomic read)
 * @param sensor_state Pointer to wheel sensor state structure
 * @return Current revolution count
 */
uint32_t wheel_sensor_get_count(const struct wheel_sensor_state *sensor_state);

/**
 * @brief ISR callback for wheel sensor trigger
 * @param dev Device pointer (unused)
 * @param cb Callback pointer (unused)
 * @param pins Triggered pins (unused)
 */
void wheel_sensor_triggered(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins);

#endif /* WHEEL_SENSOR_H */
