/**
 * @file mocks_wheel_sensor.c
 * @brief Mock implementations for wheel sensor testing
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <app.h>

/* Mock device for testing */
static const struct device *mock_gpio_dev;

/* Mock GPIO callback */
static struct gpio_callback mock_callback;

/* Mock wheel sensor state for testing */
static struct wheel_sensor_state *mock_sensor_state;

/* Mock gpio_pin_interrupt_configure function */
int mock_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
                                     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
    /* In a real test, we'd verify the parameters */
    return 0;
}

/* Mock gpio_add_callback function */
int mock_gpio_add_callback(const struct device *dev, struct gpio_callback *callback)
{
    mock_callback = *callback;
    return 0;
}

/* Mock gpio_remove_callback function */
int mock_gpio_remove_callback(const struct device *dev, struct gpio_callback *callback)
{
    return 0;
}

/* Mock device_is_ready function */
bool mock_device_is_ready(const struct device *dev)
{
    return true;
}

/* Mock k_work_submit function for testing */
void mock_k_work_submit(struct k_work *work)
{
    /* In tests, we'll handle work queue items synchronously */
    if (work && work->handler) {
        work->handler(work);
    }
}

/* Function to set the mock sensor state */
void mock_set_sensor_state(struct wheel_sensor_state *state)
{
    mock_sensor_state = state;
}

/* Function to get the mock sensor state */
struct wheel_sensor_state *mock_get_sensor_state(void)
{
    return mock_sensor_state;
}

/* Function to simulate a GPIO interrupt */
void mock_simulate_gpio_interrupt(void)
{
    if (mock_callback.handler) {
        mock_callback.handler(mock_gpio_dev, &mock_callback, 0);
    }
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    mock_gpio_dev = NULL;
    memset(&mock_callback, 0, sizeof(mock_callback));
    mock_sensor_state = NULL;
}
