/**
 * @file mocks.c
 * @brief Mock implementations for display testing
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include "app.h"

/* Mock display device */
static const struct device *mock_display_dev;

/* Mock framebuffer */
static uint8_t mock_framebuffer[128 * 64 / 8]; /* 128x64 monochrome display */

/* Mock display capabilities */
static struct display_capabilities mock_caps = {
    .x_resolution = 128,
    .y_resolution = 64,
    .supported_pixel_formats = BIT(PIXEL_FORMAT_MONO10),
    .current_pixel_format = PIXEL_FORMAT_MONO10,
    .screen_info = {
        .pixel_format = PIXEL_FORMAT_MONO10,
        .bpp = 1,
        .hsync = 0,
        .vsync = 0,
        .pitch_in_pixels = 128,
    },
};

/* Mock display_get_capabilities function */
int mock_display_get_capabilities(const struct device *dev, 
                                 struct display_capabilities *caps)
{
    if (!dev || !caps) {
        return -EINVAL;
    }
    
    *caps = mock_caps;
    return 0; /* Success */
}

/* Mock display_blanking_on function */
int mock_display_blanking_on(const struct device *dev)
{
    return 0; /* Success */
}

/* Mock display_blanking_off function */
int mock_display_blanking_off(const struct device *dev)
{
    return 0; /* Success */
}

/* Mock display_write function */
int mock_display_write(const struct device *dev, 
                      const uint16_t *data, 
                      const struct display_buffer_descriptor *desc)
{
    return 0; /* Success */
}

/* Mock device_is_ready function */
bool mock_device_is_ready(const struct device *dev)
{
    return dev != NULL;
}

/* Mock device_is_ready failure function */
bool mock_device_is_ready_fail(const struct device *dev)
{
    return false; /* Device not ready */
}

/* Function to set mock display device */
void mock_set_display_device(const struct device *dev)
{
    mock_display_dev = dev;
}

/* Function to get mock display device */
const struct device *mock_get_display_device(void)
{
    return mock_display_dev;
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    mock_display_dev = NULL;
    memset(mock_framebuffer, 0, sizeof(mock_framebuffer));
    memset(&mock_caps, 0, sizeof(mock_caps));
}