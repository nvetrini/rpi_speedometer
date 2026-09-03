/**
 * @file mocks_display.c
 * @brief Mock implementations for display testing
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <app.h>
#include <display_output.h>

/* Mock display device */
static const struct device *mock_display_dev;

/* Mock display initialization state */
static bool mock_display_initialized = false;

/* Mock display font height */
static uint8_t mock_display_font_height = 8;

/* Mock implementation of display_init */
int display_init(void)
{
    mock_display_initialized = true;
    return 0;
}

/* Mock implementation of display_is_available */
bool display_is_available(void)
{
    return mock_display_initialized && (mock_display_dev != NULL);
}

/* Mock implementation of display_get_font_height */
uint8_t display_get_font_height(void)
{
    return mock_display_font_height;
}

/* Mock implementation of display_render */
void display_render(const struct button_state *button_state,
		const struct wheel_config *wheel_config,
		const struct runtime_state *runtime_state,
		const struct battery_state *battery_state)
{
    /* Mock implementation - do nothing in tests */
}

/* Function to set mock display device */
void mock_set_display_device(const struct device *dev)
{
    mock_display_dev = dev;
}

/* Function to set mock display initialized state */
void mock_set_display_initialized(bool initialized)
{
    mock_display_initialized = initialized;
}

/* Function to set mock font height */
void mock_set_display_font_height(uint8_t height)
{
    mock_display_font_height = height;
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    mock_display_dev = NULL;
    mock_display_initialized = false;
    mock_display_font_height = 8;
}