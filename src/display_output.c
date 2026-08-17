/*
 * display_output.c - Display output module
 *
 * Handles rendering of application state to the OLED display (SSD1306).
 */

#include "display_output.h"
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#if IS_ENABLED(CONFIG_DISPLAY) && IS_ENABLED(CONFIG_CHARACTER_FRAMEBUFFER) && \
	DT_HAS_CHOSEN(zephyr_display)

static const struct device *display_dev;
static uint8_t display_font_height;
static bool display_initialized = false;

int display_init(void)
{
	int ret;
	uint8_t font_width;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		printk("Warning: display device not ready\n");
		return -ENODEV;
	}

	ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10);
	if (ret < 0) {
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01);
	}
	if (ret < 0) {
		printk("Warning: display does not support mono pixel formats: %d\n", ret);
		return ret;
	}

	ret = cfb_framebuffer_init(display_dev);
	if (ret < 0) {
		printk("Warning: failed to initialize display framebuffer: %d\n", ret);
		return ret;
	}

	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret < 0) {
		printk("Warning: failed to select default display font: %d\n", ret);
		return ret;
	}

	ret = cfb_get_font_size(display_dev, 0, &font_width, &display_font_height);
	if (ret < 0 || display_font_height == 0U) {
		display_font_height = 8U;
	}

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		printk("Warning: failed to unblank display: %d\n", ret);
	}

	display_initialized = true;
	return 0;
}

static void display_print_lines(const char *line1, const char *line2,
			const char *line3, const char *line4)
{
	int y = 0;

	if (display_dev == NULL || !display_initialized) {
		return;
	}

	cfb_framebuffer_clear(display_dev, false);

	if (line1 != NULL) {
		cfb_print(display_dev, line1, 0, y);
	}
	y += display_font_height;

	if (line2 != NULL) {
		cfb_print(display_dev, line2, 0, y);
	}
	y += display_font_height;

	if (line3 != NULL) {
		cfb_print(display_dev, line3, 0, y);
	}
	y += display_font_height;

	if (line4 != NULL) {
		cfb_print(display_dev, line4, 0, y);
	}

	cfb_framebuffer_finalize(display_dev);
}

void display_render(const struct button_state *button_state,
		const struct wheel_config *wheel_config,
		const struct runtime_state *runtime_state,
		const struct battery_state *battery_state)
{
	char line1[DISPLAY_LINE_SIZE];
	char line2[DISPLAY_LINE_SIZE];
	char line3[DISPLAY_LINE_SIZE];
	char line4[DISPLAY_LINE_SIZE];
	int speed_whole;
	unsigned int speed_frac;

	if (!display_initialized || display_dev == NULL) {
		return;
	}

	if (button_state->in_settings_mode) {
		snprintk(line1, sizeof(line1), "Settings mode");
		snprintk(line2, sizeof(line2), "Wheel: %d cm", wheel_config->diameter_cm);
		snprintk(line3, sizeof(line3), "Btn1 +1  Btn2 -1");
		snprintk(line4, sizeof(line4), "Double press B1 to exit");
	} else {
		speed_whole = (int)runtime_state->last_speed_kmh;
		speed_frac = (unsigned int)((runtime_state->last_speed_kmh - speed_whole) * 10.0f);
		if (speed_frac > 9U) {
			speed_frac = 9U;
		}

		snprintk(line1, sizeof(line1), "Speed: %d.%01u km/h", speed_whole, speed_frac);
		snprintk(line2, sizeof(line2), "Distance: %u m", runtime_state->total_distance_m);
		snprintk(line3, sizeof(line3), "Wheel: %d cm", wheel_config->diameter_cm);
		if (battery_state->valid) {
			snprintk(line4, sizeof(line4), "Battery: %d%% %.2fV",
				battery_state->percentage, (double)battery_state->voltage);
		} else {
			snprintk(line4, sizeof(line4), "Battery: n/a");
		}
	}

	display_print_lines(line1, line2, line3, line4);
}

bool display_is_available(void)
{
	return display_initialized && (display_dev != NULL);
}

uint8_t display_get_font_height(void)
{
	return display_font_height;
}

#else

int display_init(void)
{
	return 0;
}

void display_render(const struct button_state *button_state,
		const struct wheel_config *wheel_config,
		const struct runtime_state *runtime_state,
		const struct battery_state *battery_state)
{
	/* Display not enabled */
}

bool display_is_available(void)
{
	return false;
}

uint8_t display_get_font_height(void)
{
	return 8;
}

#endif /* IS_ENABLED(CONFIG_DISPLAY) && ... */
