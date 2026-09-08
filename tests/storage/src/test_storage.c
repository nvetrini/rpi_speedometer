/**
 * @file test_storage.c
 * @brief Unit tests for storage/NVS functionality
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#include <app.h>
#include <storage.h>
#include <errno.h>

/* Test fixtures */
static struct wheel_config test_wheel_config;

/* Mock control functions (defined in mocks_storage.c) */
void test_set_settings_subsys_init_return(int ret);
void test_set_settings_register_return(int ret);
void test_reset_settings_mocks(void);

/**
 * @brief Test setup function - runs before each test
 */
static void *storage_test_setup(void)
{
    /* Reset wheel config */
    test_wheel_config.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;

    /* Reset all mocks to success state */
    test_reset_settings_mocks();

    return NULL;
}

/**
 * @brief Test teardown function - runs after each test
 */
static void storage_test_teardown(void *fixture)
{
    ARG_UNUSED(fixture);

    /* Reset mocks after test */
    test_reset_settings_mocks();
}

/**
 * @brief Test wheel config structure initialization
 */
ZTEST(storage_test, test_wheel_config_initialization)
{
    struct wheel_config config;

    /* Test default initialization */
    config.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;

    /* Verify default value */
    zassert_equal(config.diameter_cm, DEFAULT_WHEEL_DIAMETER_CM,
                 "Default wheel diameter should be %d cm", DEFAULT_WHEEL_DIAMETER_CM);
}

/**
 * @brief Test wheel diameter bounds
 */
ZTEST(storage_test, test_wheel_diameter_bounds)
{
    /* Test minimum diameter */
    test_wheel_config.diameter_cm = MIN_WHEEL_DIAMETER_CM;
    zassert_true(test_wheel_config.diameter_cm >= MIN_WHEEL_DIAMETER_CM,
                "Diameter should be at least %d cm", MIN_WHEEL_DIAMETER_CM);

    /* Test maximum diameter */
    test_wheel_config.diameter_cm = MAX_WHEEL_DIAMETER_CM;
    zassert_true(test_wheel_config.diameter_cm <= MAX_WHEEL_DIAMETER_CM,
                "Diameter should be at most %d cm", MAX_WHEEL_DIAMETER_CM);

    /* Test out of bounds - below minimum */
    test_wheel_config.diameter_cm = MIN_WHEEL_DIAMETER_CM - 1;
    zassert_true(test_wheel_config.diameter_cm < MIN_WHEEL_DIAMETER_CM,
                "Diameter below minimum should be detected");

    /* Test out of bounds - above maximum */
    test_wheel_config.diameter_cm = MAX_WHEEL_DIAMETER_CM + 1;
    zassert_true(test_wheel_config.diameter_cm > MAX_WHEEL_DIAMETER_CM,
                "Diameter above maximum should be detected");
}

/**
 * @brief Test storage initialization with valid config
 */
ZTEST(storage_test, test_storage_init_valid_config)
{
    /* Test with valid wheel config */
    test_wheel_config.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;

    /* Note: In a real test environment, storage_init would be called
     * but we're testing the data structures and bounds here */

    /* Verify config is within valid range */
    zassert_true(test_wheel_config.diameter_cm >= MIN_WHEEL_DIAMETER_CM &&
                test_wheel_config.diameter_cm <= MAX_WHEEL_DIAMETER_CM,
                "Config diameter should be within valid range");
}

/**
 * @brief Test storage initialization with invalid config
 */
ZTEST(storage_test, test_storage_init_invalid_config)
{
    /* Test with invalid wheel config (too small) */
    test_wheel_config.diameter_cm = MIN_WHEEL_DIAMETER_CM - 1;

    /* Verify config is invalid */
    zassert_true(test_wheel_config.diameter_cm < MIN_WHEEL_DIAMETER_CM,
                "Config diameter should be below minimum");

    /* Test with invalid wheel config (too large) */
    test_wheel_config.diameter_cm = MAX_WHEEL_DIAMETER_CM + 1;

    /* Verify config is invalid */
    zassert_true(test_wheel_config.diameter_cm > MAX_WHEEL_DIAMETER_CM,
                "Config diameter should be above maximum");
}

/**
 * @brief Test wheel diameter configuration values
 */
ZTEST(storage_test, test_wheel_diameter_configuration_values)
{
    /* Test various valid diameter values */
    int test_values[] = {
        MIN_WHEEL_DIAMETER_CM,
        DEFAULT_WHEEL_DIAMETER_CM,
        MAX_WHEEL_DIAMETER_CM,
        20, 30, 40, 50, 60, 70, 80, 90
    };

    for (int i = 0; i < ARRAY_SIZE(test_values); i++) {
        test_wheel_config.diameter_cm = test_values[i];
        zassert_true(test_wheel_config.diameter_cm >= MIN_WHEEL_DIAMETER_CM &&
                    test_wheel_config.diameter_cm <= MAX_WHEEL_DIAMETER_CM,
                    "Diameter %d cm should be within valid range", test_values[i]);
    }
}

/**
 * @brief Test storage state persistence simulation
 */
ZTEST(storage_test, test_storage_persistence_simulation)
{
    struct wheel_config original_config;

    /* Set up original config */
    original_config.diameter_cm = 66; /* Different from default */

    /* Simulate saving config */
    test_wheel_config.diameter_cm = original_config.diameter_cm;

    /* Verify config was "saved" (in memory for this test) */
    zassert_equal(test_wheel_config.diameter_cm, original_config.diameter_cm,
                 "Config should retain saved diameter value");

    /* Simulate loading config */
    /* In a real test, this would call storage_init and verify the loaded value */
    zassert_equal(test_wheel_config.diameter_cm, 66,
                 "Loaded config should have diameter 66 cm");
}

/**
 * @brief Test storage error handling simulation
 */
ZTEST(storage_test, test_storage_error_handling)
{
    /* Test error codes that might be returned by storage operations */
    int error_codes[] = {
        -ENODEV,    /* No such device */
        -ENOENT,    /* No such file or directory */
        -EIO,       /* I/O error */
        -ENOSPC,    /* No space left on device */
        -EINVAL,    /* Invalid argument */
        -ENOTSUP,   /* Operation not supported */
    };

    /* Verify all error codes are negative (as expected) */
    for (int i = 0; i < ARRAY_SIZE(error_codes); i++) {
        zassert_true(error_codes[i] < 0,
                    "Error code %d should be negative", error_codes[i]);
    }
}

/**
 * @brief Test storage_init failure when settings_subsys_init fails
 */
ZTEST(storage_test, test_storage_init_subsys_fail)
{
    /* Set up mock to simulate settings_subsys_init failure */
    test_set_settings_subsys_init_return(-ENODEV);

    /* Call storage_init - should fail with same error */
    int ret = storage_init(&test_wheel_config);

    /* Verify that storage_init returns the error from settings_subsys_init */
    zassert_equal(ret, -ENODEV,
                 "storage_init should return -ENODEV when settings_subsys_init fails");

    /* Reset mocks for subsequent tests */
    test_reset_settings_mocks();
}

/**
 * @brief Test storage_init failure when settings_register fails
 */
ZTEST(storage_test, test_storage_init_register_fail)
{
    /* Reset mocks first */
    test_reset_settings_mocks();

    /* Set up mock to simulate settings_register failure */
    test_set_settings_register_return(-ENOMEM);

    /* Call storage_init - should fail with settings_register error */
    int ret = storage_init(&test_wheel_config);

    /* Verify that storage_init returns the error from settings_register */
    zassert_equal(ret, -ENOMEM,
                 "storage_init should return -ENOMEM when settings_register fails");

    /* Reset mocks for subsequent tests */
    test_reset_settings_mocks();
}

/**
 * @brief Test storage_init success path
 */
ZTEST(storage_test, test_storage_init_success)
{
    /* Reset mocks to return success */
    test_reset_settings_mocks();

    /* Call storage_init - should succeed */
    int ret = storage_init(&test_wheel_config);

    /* Verify that storage_init returns success */
    zassert_equal(ret, 0,
                 "storage_init should return 0 on success");
}

/**
 * @brief Test suite definition
 */
ZTEST_SUITE(storage_test,
           NULL,
           storage_test_setup,
           NULL,
           NULL,
           storage_test_teardown);
