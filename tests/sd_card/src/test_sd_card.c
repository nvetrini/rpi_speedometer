/**
 * @file test_sd_card.c
 * @brief Integration tests for filesystem operations on SD card / ramdisk
 *
 * These tests exercise the actual filesystem API using a ramdisk on native_sim.
 * Pattern based on zephyr/tests/subsys/fs/fat_fs_api/ tests.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>
#include <ff.h>

/* Test configuration */
#define TEST_MOUNT_POINT "/RAM:"
#define TEST_FILE_PATH   TEST_MOUNT_POINT "/logs/app.log"
#define TEST_DIR_PATH    TEST_MOUNT_POINT "/logs"
#define TEST_DATA        "Test log message\n"
#define TEST_DATA_LEN    (sizeof(TEST_DATA) - 1)

/* FatFS work area */
static FATFS fat_fs;

/* File object */
static struct fs_file_t file;

/* Mount structure */
static struct fs_mount_t mount = {
	.type = FS_FATFS,
	.mnt_point = TEST_MOUNT_POINT,
	.fs_data = &fat_fs,
};

/**
 * @brief Test setup - performed before each test
 */
static void *sd_card_test_setup(void)
{
	/* Initialize file object */
	fs_file_t_init(&file);
	
	/* Unmount if already mounted (clean state) */
	if (mount.fs) {
		fs_unmount(&mount);
	}
	
	/* Initialize disk access for RAM disk */
	int rc = disk_access_init("RAM");
	zassert_equal(rc, 0, "Failed to initialize RAM disk (%d)", rc);
	
	/* Mount the filesystem */
	/* With CONFIG_FS_FATFS_MOUNT_MKFS=y, it will auto-format if needed */
	rc = fs_mount(&mount);
	zassert_true(rc == 0, "Failed to mount filesystem (%d)", rc);
	
	return NULL;
}

/**
 * @brief Test teardown - performed after each test
 */
static void sd_card_test_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	
	/* Close file if open */
	if (file.mp) {
		fs_close(&file);
	}
	
	/* Unmount filesystem */
	if (mount.fs) {
		fs_unmount(&mount);
	}
}

/**
 * @brief Test filesystem mount
 */
ZTEST(sd_card_test, test_fs_mount)
{
	/* Filesystem should already be mounted by setup */
	zassert_str_equal(mount.mnt_point, TEST_MOUNT_POINT, "Mount point mismatch");
	zassert_not_null(mount.fs, "Filesystem should have fs pointer after mount");
}

/**
 * @brief Test filesystem directory creation
 */
ZTEST(sd_card_test, test_fs_mkdir)
{
	int rc;
	struct fs_dirent dirent;

	/* Create logs directory */
	rc = fs_mkdir(TEST_DIR_PATH);
	/* Accept success or EEXIST if directory already exists from previous test */
	zassert_true(rc == 0 || rc == -EEXIST, 
		"Directory creation should succeed or already exist, got %d", rc);
	
	/* Verify directory exists */
	rc = fs_stat(TEST_DIR_PATH, &dirent);
	zassert_equal(rc, 0, "Directory stat failed (%d)", rc);
	zassert_true(dirent.type == FS_DIR_ENTRY_DIR, "Path should be a directory");
}

/**
 * @brief Test file creation and writing
 */
ZTEST(sd_card_test, test_fs_file_create_write)
{
	int rc;
	ssize_t bytes_written;

	/* Create directory first */
	rc = fs_mkdir(TEST_DIR_PATH);
	/* Ignore error if directory already exists */
	
	/* Create and open file for writing */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_CREATE | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to open/create file (%d)", rc);
	
	/* Write test data */
	bytes_written = fs_write(&file, TEST_DATA, TEST_DATA_LEN);
	zassert_equal(bytes_written, TEST_DATA_LEN, 
		"Write failed: expected %d bytes, got %zd", 
		TEST_DATA_LEN, bytes_written);
	
	/* Sync to ensure data is written */
	rc = fs_sync(&file);
	zassert_equal(rc, 0, "fs_sync failed (%d)", rc);
	
	/* Close file */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "fs_close failed (%d)", rc);
}

/**
 * @brief Test file reading
 */
ZTEST(sd_card_test, test_fs_file_read)
{
	int rc;
	ssize_t bytes_read;
	char read_buffer[TEST_DATA_LEN + 1];

	/* Create directory and file with test data first */
	rc = fs_mkdir(TEST_DIR_PATH);
	
	/* Create and write file */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_CREATE | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to create file (%d)", rc);
	
	rc = fs_write(&file, TEST_DATA, TEST_DATA_LEN);
	zassert_equal(rc, TEST_DATA_LEN, "Failed to write initial data (%d)", rc);
	
	rc = fs_close(&file);
	zassert_equal(rc, 0, "Failed to close file after write (%d)", rc);
	
	/* Open file for reading */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_READ);
	zassert_equal(rc, 0, "Failed to open file for reading (%d)", rc);
	
	/* Read data back */
	bytes_read = fs_read(&file, read_buffer, TEST_DATA_LEN);
	zassert_equal(bytes_read, TEST_DATA_LEN, 
		"Read failed: expected %d bytes, got %zd", 
		TEST_DATA_LEN, bytes_read);
	
	/* Null-terminate and verify content */
	read_buffer[TEST_DATA_LEN] = '\0';
	zassert_equal(strncmp(read_buffer, TEST_DATA, TEST_DATA_LEN), 0,
		"Read data does not match written data");
	
	/* Close file */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "fs_close failed (%d)", rc);
}

/**
 * @brief Test file append
 */
ZTEST(sd_card_test, test_fs_file_append)
{
	int rc;
	ssize_t bytes_written;
	char read_buffer[TEST_DATA_LEN * 2 + 1];
	ssize_t bytes_read;
	const char *append_data = "Appended data\n";
	const size_t append_len = strlen(append_data);

	/* Create directory and initial file with test data */
	rc = fs_mkdir(TEST_DIR_PATH);
	
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_CREATE | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to create initial file (%d)", rc);
	
	rc = fs_write(&file, TEST_DATA, TEST_DATA_LEN);
	zassert_equal(rc, TEST_DATA_LEN, "Failed to write initial data (%d)", rc);
	
	rc = fs_close(&file);
	zassert_equal(rc, 0, "Failed to close file (%d)", rc);
	
	/* Open file in append mode */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_APPEND | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to open file for append (%d)", rc);
	
	/* Append data */
	bytes_written = fs_write(&file, append_data, append_len);
	zassert_equal(bytes_written, append_len, 
		"Append write failed: expected %zu bytes, got %zd", 
		append_len, bytes_written);
	
	/* Sync */
	rc = fs_sync(&file);
	zassert_equal(rc, 0, "fs_sync failed (%d)", rc);
	
	/* Close */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "fs_close failed (%d)", rc);
	
	/* Open and read entire file */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_READ);
	zassert_equal(rc, 0, "Failed to open file for reading (%d)", rc);
	
	/* Read all data */
	bytes_read = fs_read(&file, read_buffer, sizeof(read_buffer) - 1);
	zassert_true(bytes_read >= TEST_DATA_LEN + append_len, 
		"Read insufficient data: got %zd bytes, expected at least %zu",
		bytes_read, TEST_DATA_LEN + append_len);
	
	/* Verify original data is still at the beginning */
	read_buffer[bytes_read] = '\0';
	zassert_equal(strncmp(read_buffer, TEST_DATA, TEST_DATA_LEN), 0,
		"Original data corrupted after append");
	
	/* Verify appended data is at the end */
	zassert_equal(strncmp(read_buffer + (bytes_read - append_len), 
		append_data, append_len), 0,
		"Appended data not found at end of file");
	
	/* Close */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "fs_close failed (%d)", rc);
}

/**
 * @brief Test file deletion
 */
ZTEST(sd_card_test, test_fs_file_delete)
{
	int rc;
	struct fs_dirent dirent;

	/* Create a temporary file */
	const char *temp_path = TEST_MOUNT_POINT "/temp.txt";
	
	rc = fs_open(&file, temp_path, FS_O_CREATE | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to create temp file (%d)", rc);
	
	rc = fs_write(&file, "temp", 4);
	zassert_equal(rc, 4, "Failed to write to temp file (%d)", rc);
	
	rc = fs_close(&file);
	zassert_equal(rc, 0, "Failed to close temp file (%d)", rc);
	
	/* Verify file exists */
	rc = fs_stat(temp_path, &dirent);
	zassert_equal(rc, 0, "Temp file should exist");
	
	/* Delete file */
	rc = fs_unlink(temp_path);
	zassert_equal(rc, 0, "Failed to delete file (%d)", rc);
	
	/* Verify file no longer exists */
	rc = fs_stat(temp_path, &dirent);
	zassert_true(rc < 0, "File should not exist after deletion");
}

/**
 * @brief Test filesystem unmount
 */
ZTEST(sd_card_test, test_fs_unmount)
{
	int rc;

	/* Unmount the filesystem */
	rc = fs_unmount(&mount);
	zassert_equal(rc, 0, "Failed to unmount filesystem (%d)", rc);
	
	/* Verify it's no longer mounted by checking fs pointer is cleared */
	/* Note: mount.fs may not be NULL immediately after unmount in all implementations */
	/* So we just verify the unmount succeeded with rc == 0 */
	
	/* Remount for subsequent tests (setup will do this too, but be explicit) */
	rc = fs_mount(&mount);
	zassert_equal(rc, 0, "Failed to remount filesystem (%d)", rc);
}

/**
 * @brief Test error handling - opening non-existent file
 */
ZTEST(sd_card_test, test_fs_open_nonexistent)
{
	int rc;
	const char *nonexistent = TEST_MOUNT_POINT "/does_not_exist.txt";

	/* Try to open non-existent file without CREATE flag */
	rc = fs_open(&file, nonexistent, FS_O_READ);
	zassert_true(rc < 0, "Opening non-existent file should fail");
	zassert_equal(rc, -ENOENT, "Expected -ENOENT for non-existent file, got %d", rc);
}

/**
 * @brief Test error handling - writing to read-only file
 */
ZTEST(sd_card_test, test_fs_write_readonly)
{
	int rc;
	ssize_t bytes_written;

	/* Create directory first */
	rc = fs_mkdir(TEST_DIR_PATH);
	
	/* Create a file */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_CREATE | FS_O_RDWR);
	zassert_equal(rc, 0, "Failed to create file (%d)", rc);
	
	/* Write some initial data */
	rc = fs_write(&file, "init", 4);
	zassert_equal(rc, 4, "Failed to write initial data (%d)", rc);
	
	/* Close it */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "Failed to close file (%d)", rc);
	
	/* Open read-only */
	rc = fs_open(&file, TEST_FILE_PATH, FS_O_READ);
	zassert_equal(rc, 0, "Failed to open file read-only (%d)", rc);
	
	/* Try to write to read-only file */
	bytes_written = fs_write(&file, "data", 4);
	zassert_true(bytes_written < 0, "Writing to read-only file should fail");
	
	/* Close */
	rc = fs_close(&file);
	zassert_equal(rc, 0, "Failed to close file (%d)", rc);
}

/**
 * @brief Test suite definition
 */
ZTEST_SUITE(sd_card_test,
	   NULL,
	   sd_card_test_setup,
	   NULL,
	   NULL,
	   sd_card_test_teardown);
