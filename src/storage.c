/*
 * storage.c - Storage module
 *
 * Handles SD card logging and NVS (Non-Volatile Storage) for settings.
 */

#include "storage.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

/* SD card device reference */
const struct device *sd_card_dev;

/* Filesystem mount point */
const char *mount_point = "/SD:";
struct fs_mount_t sd_fs_mount = {
	.mnt_point = "/SD:",
};

/* Logging */
static struct fs_file_t log_file;
static char log_file_path[32];
static bool log_initialized = false;

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
			printk("Loaded wheel diameter: %d cm\n", val);
			return 0;
		} else {
			if (val < MIN_WHEEL_DIAMETER_CM || val > MAX_WHEEL_DIAMETER_CM) {
				printk("Invalid wheel diameter value: %d cm\n", val);
			}
			return -EINVAL;
		}
	}

	return -ENOENT;
}

int storage_init(struct wheel_config *wheel_config)
{
	wheel_config_ptr = wheel_config;
	
	/* Initialize settings subsystem */
	settings_subsys_init();
	
	/* Register settings handler */
	wheel_diameter_handler.name = "wheel_diameter";
	wheel_diameter_handler.h_set = settings_wheel_diameter_handler;
	settings_register(&wheel_diameter_handler);
	
	/* Load all settings */
	settings_load();
	
	return 0;
}

int storage_sd_init(void)
{
	/* Get the SDHC SPI device from devicetree */
	sd_card_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc0));
	if (sd_card_dev == NULL || !device_is_ready(sd_card_dev)) {
		printk("Error: SD card device not ready\n");
		return -ENODEV;
	}

	printk("SD card device found and ready\n");
	return 0;
}

int storage_log_open(void)
{
	int ret;
	char logs_dir_path[20];

	/* Initialize file object */
	fs_file_t_init(&log_file);

	/* Construct paths */
	snprintk(logs_dir_path, sizeof(logs_dir_path), "%slogs", mount_point);
	snprintk(log_file_path, sizeof(log_file_path), "%slogs/app.log", mount_point);

	/* Create logs directory if it doesn't exist */
	struct fs_dirent dir_entry;
	ret = fs_stat(logs_dir_path, &dir_entry);
	if (ret < 0 && ret != -ENOENT) {
		printk("Error: Failed to check logs directory: %d\n", ret);
		return ret;
	}

	if (ret == -ENOENT) {
		/* Directory doesn't exist, create it */
		ret = fs_mkdir(logs_dir_path);
		if (ret < 0) {
			printk("Error: Failed to create logs directory: %d\n", ret);
			return ret;
		}
		printk("Created logs directory\n");
	}

	/* Open log file for appending */
	ret = fs_open(&log_file, log_file_path,
			FS_O_WRITE | FS_O_APPEND | FS_O_CREATE);
	if (ret < 0) {
		printk("Error: Failed to open log file: %d\n", ret);
		log_file.mp = NULL;
		return ret;
	}

	printk("Opened log file at %s\n", log_file_path);
	log_initialized = true;
	return 0;
}

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
		printk("Error: Failed to write to log file: %d\n", ret);
	} else {
		/* Ensure data is written to disk */
		fs_sync(&log_file);
	}
}

void storage_save_wheel_diameter(const struct wheel_config *wheel_config)
{
	int rc = settings_save_one("wheel_diameter/wheel_diameter",
				&wheel_config->diameter_cm, sizeof(int));
	if (rc < 0) {
		printk("Failed to save wheel diameter setting: %d\n", rc);
	}
}

bool storage_log_available(void)
{
	return log_initialized && (log_file.mp != NULL);
}
