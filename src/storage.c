/*
 * storage.c - Storage module
 *
 * Handles SD card logging and NVS (Non-Volatile Storage) for settings.
 *
 * @implements REQ-HW-050, REQ-HW-051, REQ-HW-052, REQ-HW-053, REQ-SW-601, REQ-SW-602, REQ-SW-603, REQ-SW-604, REQ-SW-605, REQ-SW-606, REQ-SW-607, REQ-SW-608, REQ-SW-609
 * @tests tests/storage/src/test_storage.c, tests/sd_card/src/test_sd_card.c
 */

#include <storage.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(storage);

#ifdef CONFIG_SD_CARD_ENABLED
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs_interface.h>

/* SD card device reference */
const struct device *sd_card_dev;

/* Filesystem mount point */
const char *mount_point = "/SD:";
struct fs_mount_t sd_fs_mount = {
	.mnt_point = "/SD:",
};
#endif

#ifdef CONFIG_SD_CARD_ENABLED
/* Logging */
static struct fs_file_t log_file;
static char log_file_path[32];
static bool log_initialized = false;
#endif

/* Settings handler */
static struct settings_handler wheel_diameter_handler;
static struct wheel_config *wheel_config_ptr = NULL;

static int settings_wheel_diameter_handler(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	int val;

	if (strcmp(key, "wheel_diameter") == 0) {
		if (len != sizeof(int)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &val, sizeof(int));
		if (rc < 0) {
			return rc;
		}

		if (val >= MIN_WHEEL_DIAMETER_CM && val <= MAX_WHEEL_DIAMETER_CM && wheel_config_ptr != NULL) {
			wheel_config_ptr->diameter_cm = val;
			LOG_INF("Loaded wheel diameter: %d cm", val);
			return 0;
		} else {
			if (val < MIN_WHEEL_DIAMETER_CM || val > MAX_WHEEL_DIAMETER_CM) {
				LOG_WRN("Invalid wheel diameter value: %d cm", val);
			}
			return -EINVAL;
		}
	}

	return -ENOENT;
}

/**
 * @brief Initialize storage and NVS subsystem
 * @implements REQ-SW-601, REQ-SW-602, REQ-SW-603
 * @param wheel_config Pointer to wheel configuration
 * @return 0 on success, negative errno on failure
 */
int storage_init(struct wheel_config *wheel_config)
{
	wheel_config_ptr = wheel_config;

	/* Initialize settings subsystem */
	int ret = settings_subsys_init();
	if (ret < 0) {
	  LOG_ERR("Settings subsystem initialization failed: %d", -ret);
      return ret;
	}

	/* Register settings handler */
	wheel_diameter_handler.name = "wheel_diameter";
	wheel_diameter_handler.h_set = settings_wheel_diameter_handler;
	ret = settings_register(&wheel_diameter_handler);
	if (ret < 0) {
	    LOG_ERR("Failed to register wheel settings handler: %d", -ret);
		return ret;
	}

	/* Load all settings */
	ret = settings_load();
	if (ret < 0) {
	  LOG_ERR("Failed to read settings: %d", -ret);
      return ret;
	}

	return 0;
}

#ifdef CONFIG_SD_CARD_ENABLED
/**
 * @brief Initialize SD card device and filesystem
 * @implements REQ-HW-050, REQ-HW-051, REQ-SW-604
 * @return 0 on success, negative errno on failure
 */
int storage_sd_init(void)
{
	/* Get the SDHC SPI device from devicetree */
	sd_card_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc0));
	if (sd_card_dev == NULL || !device_is_ready(sd_card_dev)) {
		LOG_ERR("SD card device not ready");
		return -ENODEV;
	}

	/* Mount the filesystem */
	int ret = fs_mount(&sd_fs_mount);
	if (ret < 0) {
		LOG_ERR("Failed to mount filesystem: %d", ret);
		return ret;
	}

	LOG_INF("SD card device found, mounted, and ready");
	return 0;
}
#endif

#ifdef CONFIG_SD_CARD_ENABLED
/**
 * @brief Open log file for appending
 * @implements REQ-SW-605, REQ-SW-606, REQ-SW-607
 * @return 0 on success, negative errno on failure
 */
int storage_log_open(void)
{
	int ret;
	char logs_dir_path[20];

	/* Close any previously opened log file */
	if (log_file.mp != NULL) {
		fs_close(&log_file);
	}

	/* Initialize file object */
	fs_file_t_init(&log_file);

	/* Construct paths */
	snprintk(logs_dir_path, sizeof(logs_dir_path), "%slogs", mount_point);
	snprintk(log_file_path, sizeof(log_file_path), "%slogs/app.log", mount_point);

	/* Create logs directory if it doesn't exist */
	struct fs_dirent dir_entry;
	ret = fs_stat(logs_dir_path, &dir_entry);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("Failed to check logs directory: %d", ret);
		return ret;
	}

	if (ret == -ENOENT) {
		/* Directory doesn't exist, create it */
		ret = fs_mkdir(logs_dir_path);
		if (ret < 0) {
			LOG_ERR("Failed to create logs directory: %d", ret);
			return ret;
		}
		LOG_INF("Created logs directory");
	}

	/* Open log file for appending */
	ret = fs_open(&log_file, log_file_path,
			FS_O_WRITE | FS_O_APPEND | FS_O_CREATE);
	if (ret < 0) {
		LOG_ERR("Failed to open log file: %d", ret);
		log_file.mp = NULL;
		return ret;
	}

	LOG_INF("Opened log file at %s", log_file_path);
	log_initialized = true;
	return 0;
}
#endif

#ifdef CONFIG_SD_CARD_ENABLED
/**
 * @brief Write message to log file
 * @implements REQ-SW-607
 * @param msg Message to write to log
 */
void storage_log_write(const char *msg)
{
	if (!log_initialized || log_file.mp == NULL) {
		return;
	}

	int ret;
	size_t len = strlen(msg);

	/* Write the message to the log file */
	ret = fs_write(&log_file, msg, len);
	if (ret < 0) {
		LOG_ERR("Failed to write to log file: %d", ret);
	} else {
		/* Ensure data is written to disk */
		fs_sync(&log_file);
	}
}
#endif

/**
 * @brief Save wheel diameter to NVS
 * @implements REQ-SW-601, REQ-SW-609
 * @param wheel_config Pointer to wheel configuration
 */
void storage_save_wheel_diameter(const struct wheel_config *wheel_config)
{
	int rc = settings_save_one("wheel_diameter/wheel_diameter",
				&wheel_config->diameter_cm, sizeof(int));
	if (rc < 0) {
		LOG_ERR("Failed to save wheel diameter setting: %d", rc);
	}
}

#ifdef CONFIG_SD_CARD_ENABLED
/**
 * @brief Check if logging is available
 * @implements REQ-SW-608
 * @return true if logging is available, false otherwise
 */
bool storage_log_available(void)
{
	return log_initialized && (log_file.mp != NULL);
}
#endif
