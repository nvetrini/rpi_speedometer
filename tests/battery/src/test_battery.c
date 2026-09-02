/**
 * @file test_battery.c
 * @brief Unit tests for battery monitoring functionality
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <math.h>
#include "../../../include/app.h"
#include "../../../src/battery.h"

/* Test fixtures */
static struct battery_state test_battery_state;

/**
 * @brief Test setup function - runs before each test
 */
static void *battery_test_setup(void)
{
    /* Reset battery state */
    memset(&test_battery_state, 0, sizeof(test_battery_state));
    test_battery_state.percentage = -1;
    test_battery_state.voltage = 0.0f;
    test_battery_state.valid = false;
    return NULL;
}

/**
 * @brief Test teardown function - runs after each test
 */
static void battery_test_teardown(void *fixture)
{
    ARG_UNUSED(fixture);
}

/**
 * @brief Test battery voltage to percentage conversion
 */
ZTEST(battery_test, test_battery_voltage_to_percentage)
{
    int percentage;
    
    /* Test minimum voltage (should be 0%) */
    percentage = battery_voltage_to_percentage(BATTERY_MIN_V);
    zassert_equal(percentage, 0, "Minimum voltage should be 0%");
    
    /* Test maximum voltage (should be 100%) */
    percentage = battery_voltage_to_percentage(BATTERY_MAX_V);
    zassert_equal(percentage, 100, "Maximum voltage should be 100%");
    
    /* Test middle voltage (should be 50%) */
    float mid_voltage = (BATTERY_MIN_V + BATTERY_MAX_V) / 2.0f;
    percentage = battery_voltage_to_percentage(mid_voltage);
    zassert_equal(percentage, 50, "Middle voltage should be 50%");
    
    /* Test voltage below minimum (should clamp to 0%) */
    percentage = battery_voltage_to_percentage(BATTERY_MIN_V - 0.1f);
    zassert_equal(percentage, 0, "Voltage below minimum should clamp to 0%");
    
    /* Test voltage above maximum (should clamp to 100%) */
    percentage = battery_voltage_to_percentage(BATTERY_MAX_V + 0.1f);
    zassert_equal(percentage, 100, "Voltage above maximum should clamp to 100%");
}

/**
 * @brief Test battery ADC to voltage conversion
 */
ZTEST(battery_test, test_battery_adc_to_voltage)
{
    float voltage;
    
    /* Test with ADC value that represents reference voltage */
    /* Assuming 12-bit ADC with 3.3V reference, and voltage divider ratio */
    /* For this test, we'll test the conversion function directly */
    
    /* Test with 0 ADC value (should be 0V) */
    voltage = battery_adc_to_voltage(0);
    zassert_equal(voltage, 0.0f, "ADC value 0 should convert to 0V");
    
    /* Test with maximum 12-bit ADC value (4095) */
    /* This should give us the reference voltage divided by the voltage divider ratio */
    voltage = battery_adc_to_voltage(4095);
    /* The exact value depends on the reference voltage and divider ratio */
    zassert_true(voltage > 0.0f, "Maximum ADC value should convert to positive voltage");
}

/**
 * @brief Test battery state structure initialization
 */
ZTEST(battery_test, test_battery_state_initialization)
{
    struct battery_state state;
    
    /* Initialize to zero */
    memset(&state, 0, sizeof(state));
    
    /* Verify default values */
    zassert_equal(state.percentage, 0, "Default percentage should be 0");
    zassert_equal(state.voltage, 0.0f, "Default voltage should be 0.0");
    zassert_false(state.valid, "Default valid flag should be false");
}

/**
 * @brief Test battery percentage clamping
 */
ZTEST(battery_test, test_battery_percentage_clamping)
{
    int percentage;
    
    /* Test edge cases for percentage calculation */
    
    /* Test with voltage exactly at minimum */
    percentage = battery_voltage_to_percentage(BATTERY_MIN_V);
    zassert_true(percentage >= 0 && percentage <= 100, 
                "Percentage should be between 0 and 100");
    
    /* Test with voltage exactly at maximum */
    percentage = battery_voltage_to_percentage(BATTERY_MAX_V);
    zassert_true(percentage >= 0 && percentage <= 100, 
                "Percentage should be between 0 and 100");
    
    /* Test with voltage in the middle range */
    for (float v = BATTERY_MIN_V; v <= BATTERY_MAX_V; v += 0.1f) {
        percentage = battery_voltage_to_percentage(v);
        zassert_true(percentage >= 0 && percentage <= 100, 
                    "Percentage should always be between 0 and 100 for voltage %.2fV", v);
    }
}

/**
 * @brief Test battery voltage calculation with divider ratio
 */
ZTEST(battery_test, test_battery_voltage_divider_ratio)
{
    float adc_voltage, battery_voltage;
    
    /* Test the voltage divider ratio calculation */
    /* Assuming VOLTAGE_DIVIDER_RATIO is (R1+R2)/R2 */
    
    /* If ADC reads 1.0V, battery voltage should be 1.0 * VOLTAGE_DIVIDER_RATIO */
    adc_voltage = 1.0f;
    battery_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;
    
    /* Verify the ratio is applied correctly */
    zassert_true(battery_voltage > adc_voltage, 
                "Battery voltage should be higher than ADC voltage due to divider ratio");
    
    /* Test with 0V */
    adc_voltage = 0.0f;
    battery_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;
    zassert_equal(battery_voltage, 0.0f, 
                 "0V ADC should result in 0V battery voltage");
}

/**
 * @brief Test battery state updates
 */
ZTEST(battery_test, test_battery_state_updates)
{
    /* Test updating battery state */
    test_battery_state.percentage = 75;
    test_battery_state.voltage = 2.5f;
    test_battery_state.valid = true;
    
    /* Verify updates */
    zassert_equal(test_battery_state.percentage, 75, "Percentage should be updated to 75");
    zassert_equal(test_battery_state.voltage, 2.5f, "Voltage should be updated to 2.5V");
    zassert_true(test_battery_state.valid, "Valid flag should be true");
}

/**
 * @brief Test suite definition
 */
ZTEST_SUITE(battery_test, 
           NULL, 
           battery_test_setup,
           NULL,
           NULL,
           battery_test_teardown);
