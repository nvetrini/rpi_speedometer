#ifndef TEST_WHEEL_SENSOR_H
#define TEST_WHEEL_SENSOR_H

#include <ztest.h>
#include <zephyr/kernel.h>

/* Test helper functions */

/**
 * @brief Mock GPIO callback for testing
 */
void test_wheel_sensor_mock_callback(const struct device *dev,
                                   struct gpio_callback *cb, uint32_t pins);

/**
 * @brief Reset wheel sensor state for testing
 */
void test_wheel_sensor_reset_state(struct wheel_sensor_state *state);

/**
 * @brief Simulate wheel sensor trigger
 */
void test_wheel_sensor_simulate_trigger(struct wheel_sensor_state *state);

#endif /* TEST_WHEEL_SENSOR_H */