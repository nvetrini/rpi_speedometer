/*
 * display_output.h - Display output module interface
 *
 * Handles rendering of application state to the OLED display.
 */

#ifndef DISPLAY_OUTPUT_H
#define DISPLAY_OUTPUT_H

#include "app.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize display
 * @return 0 on success, negative errno on failure
 */
int display_init(void);

/**
 * @brief Render current status to display
 * @param button_state Pointer to button state
 * @param wheel_config Pointer to wheel configuration
 * @param runtime_state Pointer to runtime state
 * @param battery_state Pointer to battery state
 */
void display_render(const struct button_state *button_state,
		const struct wheel_config *wheel_config,
		const struct runtime_state *runtime_state,
		const struct battery_state *battery_state);

/**
 * @brief Check if display is available
 * @return true if display is initialized and ready
 */
bool display_is_available(void);

/**
 * @brief Get display font height
 * @return Font height in pixels
 */
uint8_t display_get_font_height(void);

#endif /* DISPLAY_OUTPUT_H */
