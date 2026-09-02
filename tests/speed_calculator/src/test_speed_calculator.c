/**
 * @file test_speed_calculator.c
 * @brief Unit tests for speed calculator functionality
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <app.h>
#include <speed_calculator.h>

/* Test fixtures */
static struct runtime_state test_runtime_state;

/**
 * @brief Test setup function - runs before each test
 */
static void *speed_calculator_test_setup(void)
{
    /* Reset runtime state */
    memset(&test_runtime_state, 0, sizeof(test_runtime_state));
    return NULL;
}

/**
 * @brief Test teardown function - runs after each test
 */
static void speed_calculator_test_teardown(void *fixture)
{
    ARG_UNUSED(fixture);
}

/**
 * @brief Test speed calculation with zero revolutions
 */
ZTEST(speed_calculator_test, test_speed_calculation_zero_revolutions)
{
    uint32_t current_count = 0;
    uint32_t last_count = 0;
    int wheel_diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;
    uint32_t interval_ms = REPORT_INTERVAL_MS;
    
    /* Calculate speed and distance */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, interval_ms,
        &test_runtime_state.total_distance_m, &test_runtime_state.last_speed_kmh);
    
    /* Verify results */
    zassert_equal(test_runtime_state.last_speed_kmh, 0.0f,
                 "Speed should be 0 km/h with no revolutions");
    zassert_equal(test_runtime_state.total_distance_m, 0,
                 "Distance should be 0 m with no revolutions");
}

/**
 * @brief Test speed calculation with constant revolutions
 */
ZTEST(speed_calculator_test, test_speed_calculation_constant_revolutions)
{
    uint32_t current_count = 10;
    uint32_t last_count = 10;
    int wheel_diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;
    uint32_t interval_ms = REPORT_INTERVAL_MS;
    
    /* Calculate speed and distance */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, interval_ms,
        &test_runtime_state.total_distance_m, &test_runtime_state.last_speed_kmh);
    
    /* Verify results */
    zassert_equal(test_runtime_state.last_speed_kmh, 0.0f,
                 "Speed should be 0 km/h with no new revolutions");
}

/**
 * @brief Test speed calculation with increasing revolutions
 */
ZTEST(speed_calculator_test, test_speed_calculation_increasing_revolutions)
{
    uint32_t current_count = 100;
    uint32_t last_count = 50;
    int wheel_diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;
    uint32_t interval_ms = REPORT_INTERVAL_MS;
    
    /* Calculate speed and distance */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, interval_ms,
        &test_runtime_state.total_distance_m, &test_runtime_state.last_speed_kmh);
    
    /* Verify results - should have positive speed and distance */
    zassert_true(test_runtime_state.last_speed_kmh > 0.0f,
                "Speed should be positive with increasing revolutions");
    zassert_true(test_runtime_state.total_distance_m > 0,
                "Distance should be positive with revolutions");
}

/**
 * @brief Test speed calculation with different wheel diameters
 */
ZTEST(speed_calculator_test, test_speed_calculation_different_diameters)
{
    uint32_t current_count = 100;
    uint32_t last_count = 0;
    uint32_t interval_ms = REPORT_INTERVAL_MS;
    float speed_small, speed_large;
    uint32_t distance_small, distance_large;
    
    /* Test with small wheel diameter */
    speed_calculator_update(
        current_count, last_count,
        MIN_WHEEL_DIAMETER_CM, interval_ms,
        &distance_small, &speed_small);
    
    /* Test with large wheel diameter */
    speed_calculator_update(
        current_count, last_count,
        MAX_WHEEL_DIAMETER_CM, interval_ms,
        &distance_large, &speed_large);
    
    /* Verify that larger diameter results in higher speed and distance */
    zassert_true(speed_large > speed_small,
                "Larger wheel diameter should result in higher speed");
    zassert_true(distance_large > distance_small,
                "Larger wheel diameter should result in greater distance");
}

/**
 * @brief Test speed calculation with different intervals
 */
ZTEST(speed_calculator_test, test_speed_calculation_different_intervals)
{
    uint32_t current_count = 100;
    uint32_t last_count = 0;
    int wheel_diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;
    float speed_short, speed_long;
    uint32_t distance_short, distance_long;
    
    /* Test with short interval */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, 500, /* 500ms interval */
        &distance_short, &speed_short);
    
    /* Test with long interval */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, 2000, /* 2000ms interval */
        &distance_long, &speed_long);
    
    /* Verify that longer interval results in lower speed (same revolutions over longer time) */
    zassert_true(speed_short > speed_long,
                "Shorter interval should result in higher speed");
    
    /* Distance should be the same regardless of interval (based on revolutions) */
    zassert_equal(distance_short, distance_long,
                 "Distance should be the same regardless of interval");
}

/**
 * @brief Test speed calculation accumulation over multiple updates
 */
ZTEST(speed_calculator_test, test_speed_calculation_accumulation)
{
    uint32_t current_count = 0;
    uint32_t last_count = 0;
    int wheel_diameter_cm = DEFAULT_WHEEL_DIAMETER_CM;
    uint32_t interval_ms = REPORT_INTERVAL_MS;
    uint32_t total_distance = 0;
    float last_speed = 0.0f;
    
    /* Simulate multiple updates with increasing revolutions */
    for (int i = 1; i <= 10; i++) {
        current_count = i * 10; /* 10, 20, 30, ..., 100 revolutions */
        
        speed_calculator_update(
            current_count, last_count,
            wheel_diameter_cm, interval_ms,
            &total_distance, &last_speed);
        
        last_count = current_count;
    }
    
    /* Verify results */
    zassert_true(total_distance > 0,
                "Total distance should be positive after multiple updates");
    zassert_true(last_speed >= 0.0f,
                "Last speed should be non-negative");
}

/**
 * @brief Test speed calculation with realistic values
 */
ZTEST(speed_calculator_test, test_speed_calculation_realistic_values)
{
    uint32_t current_count = 100;
    uint32_t last_count = 0;
    int wheel_diameter_cm = 66; /* Typical 26" wheel diameter in cm */
    uint32_t interval_ms = 1000; /* 1 second */
    
    /* Calculate speed and distance */
    speed_calculator_update(
        current_count, last_count,
        wheel_diameter_cm, interval_ms,
        &test_runtime_state.total_distance_m, &test_runtime_state.last_speed_kmh);
    
    /* Verify results are reasonable */
    zassert_true(test_runtime_state.last_speed_kmh > 0.0f,
                "Speed should be positive");
    zassert_true(test_runtime_state.last_speed_kmh < 200.0f,
                "Speed should be reasonable (< 200 km/h)");
    zassert_true(test_runtime_state.total_distance_m > 0,
                "Distance should be positive");
}

/**
 * @brief Test runtime state structure initialization
 */
ZTEST(speed_calculator_test, test_runtime_state_initialization)
{
    struct runtime_state state;
    
    /* Initialize to zero */
    memset(&state, 0, sizeof(state));
    
    /* Verify default values */
    zassert_equal(state.last_count, 0,
                 "Default last count should be 0");
    zassert_equal(state.total_distance_m, 0,
                 "Default total distance should be 0");
    zassert_equal(state.last_speed_kmh, 0.0f,
                 "Default last speed should be 0.0");
}

/**
 * @brief Test suite definition
 */
ZTEST_SUITE(speed_calculator_test, 
           NULL, 
           speed_calculator_test_setup,
           NULL,
           NULL,
           speed_calculator_test_teardown);
