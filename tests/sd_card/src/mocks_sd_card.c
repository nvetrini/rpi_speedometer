/**
 * @file mocks_sd_card.c
 * @brief Mock implementations for SD card testing
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>
#include "../../../include/app.h"

/* Mock SD card device */
static const struct device *mock_sd_card_dev;

/* Mock filesystem mount */
static struct fs_mount_t mock_sd_fs_mount = {
    .mnt_point = "/SD:",
};

/* Mock file object */
static struct fs_file_t mock_log_file;

/* Mock filesystem stat buffer */
static struct fs_dirent mock_dirent;

/* Mock fs_mount function */
int mock_fs_mount(struct fs_mount_t *mp)
{
    if (!mp) {
        return -EINVAL;
    }
    
    *mp = mock_sd_fs_mount;
    return 0; /* Success */
}

/* Mock fs_mount failure function */
int mock_fs_mount_fail(struct fs_mount_t *mp)
{
    return -ENODEV; /* No such device */
}

/* Mock fs_open function */
int mock_fs_open(struct fs_file_t *file, const char *path, int flags)
{
    if (!file || !path) {
        return -EINVAL;
    }
    
    *file = mock_log_file;
    file->mp = &mock_sd_fs_mount;
    return 0; /* Success */
}

/* Mock fs_open failure function */
int mock_fs_open_fail(struct fs_file_t *file, const char *path, int flags)
{
    return -EACCES; /* Permission denied */
}

/* Mock fs_write function */
int mock_fs_write(struct fs_file_t *file, const void *data, size_t len)
{
    if (!file || !data || len == 0) {
        return -EINVAL;
    }
    
    return len; /* Return number of bytes written */
}

/* Mock fs_write failure function */
int mock_fs_write_fail(struct fs_file_t *file, const void *data, size_t len)
{
    return -ENOSPC; /* No space left on device */
}

/* Mock fs_close function */
int mock_fs_close(struct fs_file_t *file)
{
    return 0; /* Success */
}

/* Mock fs_sync function */
int mock_fs_sync(struct fs_file_t *file)
{
    return 0; /* Success */
}

/* Mock fs_mkdir function */
int mock_fs_mkdir(const char *path)
{
    if (!path) {
        return -EINVAL;
    }
    return 0; /* Success */
}

/* Mock fs_mkdir failure function */
int mock_fs_mkdir_fail(const char *path)
{
    return -EACCES; /* Permission denied */
}

/* Mock fs_stat function */
int mock_fs_stat(const char *path, struct fs_dirent *entry)
{
    if (!path || !entry) {
        return -EINVAL;
    }
    
    /* Simulate directory not existing */
    return -ENOENT;
}

/* Mock fs_stat function for existing directory */
int mock_fs_stat_exists(const char *path, struct fs_dirent *entry)
{
    if (!path || !entry) {
        return -EINVAL;
    }
    
    /* Simulate directory existing */
    *entry = mock_dirent;
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

/* Function to set mock SD card device */
void mock_set_sd_card_device(const struct device *dev)
{
    mock_sd_card_dev = dev;
}

/* Function to get mock SD card device */
const struct device *mock_get_sd_card_device(void)
{
    return mock_sd_card_dev;
}

/* Function to set mock filesystem behavior */
void mock_set_fs_behavior(int behavior)
{
    /* Can be extended to simulate different filesystem behaviors */
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    mock_sd_card_dev = NULL;
    memset(&mock_sd_fs_mount, 0, sizeof(mock_sd_fs_mount));
    mock_sd_fs_mount.mnt_point = "/SD:";
    memset(&mock_log_file, 0, sizeof(mock_log_file));
    memset(&mock_dirent, 0, sizeof(mock_dirent));
}
