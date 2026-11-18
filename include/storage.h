/*
 * storage.h - Storage module interface
 *
 * Handles SD card logging and NVS (Non-Volatile Storage) for settings.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <app.h>
#include <stdbool.h>
#include <zephyr/settings/settings.h>

/* Settings subtree name */
#define SETTINGS_SUBTREE "wheel_diameter"

/* SD card related declarations - only available when SD card support is enabled */
#ifdef CONFIG_SD_CARD_ENABLED
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>

/* Forward declarations for SD card device */
extern const struct device *sd_card_dev;
extern const char *mount_point;
extern struct fs_mount_t sd_fs_mount;
#endif

/**
 * @brief Initialize storage (settings subsystem)
 * @param wheel_config Pointer to wheel configuration (for settings handler)
 * @return 0 on success, negative errno on failure
 */
int storage_init(struct wheel_config *wheel_config);

/**
 * @brief Initialize SD card filesystem
 * @return 0 on success, negative errno on failure
 */
#ifdef CONFIG_SD_CARD_ENABLED
int storage_sd_init(void);
#else
static inline int storage_sd_init(void) { return -ENOSYS; }
#endif

/**
 * @brief Open log file
 * @return 0 on success, negative errno on failure
 */
#ifdef CONFIG_SD_CARD_ENABLED
int storage_log_open(void);
#else
static inline int storage_log_open(void) { return -ENOSYS; }
#endif

/**
 * @brief Log message to SD card
 * @param msg Message to log (null-terminated string)
 */
#ifdef CONFIG_SD_CARD_ENABLED
void storage_log_write(const char *msg);
#else
static inline void storage_log_write(const char *msg) { }
#endif

/**
 * @brief Save wheel diameter to NVS
 * @param wheel_config Pointer to wheel configuration to save
 */
void storage_save_wheel_diameter(const struct wheel_config *wheel_config);

/**
 * @brief Check if SD card logging is available
 * @return true if log file is open
 */
#ifdef CONFIG_SD_CARD_ENABLED
bool storage_log_available(void);
#else
static inline bool storage_log_available(void) { return false; }
#endif

#endif /* STORAGE_H */
