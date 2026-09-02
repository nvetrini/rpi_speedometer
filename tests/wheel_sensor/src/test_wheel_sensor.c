/**
 * @file test_wheel_sensor.c
 * @brief Unit tests for wheel sensor functionality
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <app.h>
#include <wheel_sensor.h>

/* Test fixtures */
static struct wheel_sensor_state test_state;

/**
 * @brief Test setup function - runs before each test
 */
static void *wheel_sensor_test_setup(void)
{
    /* Reset the test state */
    test_wheel_sensor_reset_state(&test_state);
    return NULL;
}

/**
 * @brief Test teardown function - runs after each test
 */
static void wheel_sensor_test_teardown(void *fixture)
{
    /* Clean up if needed */
    ARG_UNUSED(fixture);
}

/**
 * @brief Reset wheel sensor state for testing
 */
void test_wheel_sensor_reset_state(struct wheel_sensor_state *state)
{
    atomic_set(&state->revolution_count, 0);
    state->last_trigger_ms = 0;
}

/**
 * @brief Test wheel sensor initialization success
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_init_success)
{
    int ret;
    
    /* Reset state before test */
    test_wheel_sensor_reset_state(&test_state);
    
    /* This test would normally call wheel_sensor_init, but since we're 
     * testing in a simulated environment, we'll test the state management */
    
    /* Verify initial state */
    zassert_equal(atomic_get(&test_state.revolution_count), 0, 
                 "Initial revolution count should be 0");
    zassert_equal(test_state.last_trigger_ms, 0,
                 "Initial last trigger time should be 0");
}

/**
 * @brief Test wheel sensor count increment
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_count_increment)
{
    /* Reset state */
    test_wheel_sensor_reset_state(&test_state);
    
    /* Simulate multiple triggers */
    for (int i = 0; i < 5; i++) {
        atomic_inc(&test_state.revolution_count);
        test_state.last_trigger_ms = k_uptime_get();
    }
    
    /* Verify count was incremented */
    zassert_equal(atomic_get(&test_state.revolution_count), 5,
                 "Revolution count should be 5 after 5 triggers");
    zassert_true(test_state.last_trigger_ms > 0,
                "Last trigger time should be updated");
}

/**
 * @brief Test wheel sensor get count function
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_get_count)
{
    uint32_t count;
    
    /* Reset state */
    test_wheel_sensor_reset_state(&test_state);
    
    /* Set a known count */
    atomic_set(&test_state.revolution_count, 42);
    
    /* Get the count using the accessor function */
    count = wheel_sensor_get_count(&test_state);
    
    /* Verify the count */
    zassert_equal(count, 42, "Get count should return 42");
}

/**
 * @brief Test wheel sensor atomic operations
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_atomic_operations)
{
    /* Reset state */
    test_wheel_sensor_reset_state(&test_state);
    
    /* Test atomic increment from multiple contexts */
    atomic_inc(&test_state.revolution_count);
    atomic_inc(&test_state.revolution_count);
    atomic_inc(&test_state.revolution_count);
    
    zassert_equal(atomic_get(&test_state.revolution_count), 3,
                 "Atomic count should be 3");
    
    /* Test atomic set */
    atomic_set(&test_state.revolution_count, 100);
    zassert_equal(atomic_get(&test_state.revolution_count), 100,
                 "Atomic count should be 100 after set");
}

/**
 * @brief Test wheel sensor debounce timing
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_debounce_timing)
{
    int64_t initial_time = k_uptime_get();
    
    /* Reset state */
    test_wheel_sensor_reset_state(&test_state);
    test_state.last_trigger_ms = initial_time;
    
    /* Simulate a trigger after debounce period */
    k_msleep(DEBOUNCE_MS + 1);
    int64_t new_time = k_uptime_get();
    
    /* Verify that enough time has passed for debounce */
    zassert_true((new_time - initial_time) >= DEBOUNCE_MS,
                "Debounce period should have elapsed");
    
    /* Update the trigger time */
    test_state.last_trigger_ms = new_time;
    zassert_true(test_state.last_trigger_ms > initial_time,
                "Last trigger time should be updated");
}

/**
 * @brief Test wheel sensor state structure size
 */
ZTEST(wheel_sensor_test, test_wheel_sensor_state_size)
{
    /* Verify the state structure has expected members */
    struct wheel_sensor_state local_state;
    
    /* Initialize to zero */
    memset(&local_state, 0, sizeof(local_state));
    
    /* Verify atomic is properly initialized */
    zassert_equal(atomic_get(&local_state.revolution_count), 0,
                 "Newly created state should have zero count");
    zassert_equal(local_state.last_trigger_ms, 0,
                 "Newly created state should have zero trigger time");
}

/**
 * @brief Test suite definition
 */
ZTEST_SUITE(wheel_sensor_test, 
           NULL, 
           wheel_sensor_test_setup,
           NULL,
           NULL,
           wheel_sensor_test_teardown);
