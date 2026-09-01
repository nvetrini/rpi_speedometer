# Tests for wheel sensor speedometer application
# Uses Zephyr's ztest framework

# Include the Zephyr testing framework
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Add test directories
add_subdirectory_ifdef(CONFIG_TEST_WHEEL_SENSOR test_wheel_sensor)
add_subdirectory_ifdef(CONFIG_TEST_BATTERY test_battery)
add_subdirectory_ifdef(CONFIG_TEST_STORAGE test_storage)
add_subdirectory_ifdef(CONFIG_TEST_SD_CARD test_sd_card)
add_subdirectory_ifdef(CONFIG_TEST_SPEED_CALCULATOR test_speed_calculator)
add_subdirectory_ifdef(CONFIG_TEST_DISPLAY test_display)

# Common test configuration
set(TEST_APP_NAME "wheel_sensor_tests")

# Enable all tests by default when building for testing
if(CONFIG_TEST)
    # Include all test modules
    add_subdirectory(test_wheel_sensor)
    add_subdirectory(test_battery)
    add_subdirectory(test_storage)
    add_subdirectory(test_sd_card)
    add_subdirectory(test_speed_calculator)
    add_subdirectory(test_display)
endif()