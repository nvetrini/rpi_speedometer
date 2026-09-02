/**
 * @file mocks_storage.c
 * @brief Mock implementations for storage testing
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#include <app.h>

/* Mock settings handler */
static struct settings_handler mock_settings_handler;

/* Mock flash device */
static const struct device *mock_flash_dev;

/* Mock filesystem mount */
static struct fs_mount_t mock_mount;

/* Mock file object */
static struct fs_file_t mock_file;

/* Mock settings_load function */
int mock_settings_load(void)
{
    return 0; /* Success */
}

/* Mock settings_save_one function */
int mock_settings_save_one(const char *name, const void *value, size_t len)
{
    return 0; /* Success */
}

/* Mock settings_register function */
int mock_settings_register(struct settings_handler *handler)
{
    mock_settings_handler = *handler;
    return 0; /* Success */
}

/* Mock settings_subsys_init function */
void mock_settings_subsys_init(void)
{
    /* Do nothing in mock */
}

/* Mock fs_mount function */
int mock_fs_mount(struct fs_mount_t *mp)
{
    *mp = mock_mount;
    return 0; /* Success */
}

/* Mock fs_open function */
int mock_fs_open(struct fs_file_t *file, const char *path, int flags)
{
    *file = mock_file;
    return 0; /* Success */
}

/* Mock fs_write function */
int mock_fs_write(struct fs_file_t *file, const void *data, size_t len)
{
    return len; /* Return number of bytes written */
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
    return 0; /* Success */
}

/* Mock fs_stat function */
int mock_fs_stat(const char *path, struct fs_dirent *entry)
{
    return -ENOENT; /* File not found by default */
}

/* Mock device_is_ready function */
bool mock_device_is_ready(const struct device *dev)
{
    return dev != NULL;
}

/* Function to set mock filesystem behavior */
void mock_set_fs_behavior(int behavior)
{
    /* Can be extended to simulate different filesystem behaviors */
}

/* Function to set mock settings behavior */
void mock_set_settings_behavior(int behavior)
{
    /* Can be extended to simulate different settings behaviors */
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    memset(&mock_settings_handler, 0, sizeof(mock_settings_handler));
    mock_flash_dev = NULL;
    memset(&mock_mount, 0, sizeof(mock_mount));
    memset(&mock_file, 0, sizeof(mock_file));
}
