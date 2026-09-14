cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wheel_sensor)

# Include directories
target_include_directories(app PRIVATE include src)

# Main application and module sources
target_sources(app PRIVATE
    src/main.c
    src/app.c
    src/wheel_sensor.c
    src/button.c
    src/speed_calculator.c
    src/display_output.c
    src/console_output.c
    src/storage.c
)

# Battery monitoring (optional)
if(CONFIG_BATTERY_ENABLED)
    target_sources(app PRIVATE src/battery.c)
endif()

# Simulation shell (optional)
if(CONFIG_APP_SIMULATION)
    target_sources(app PRIVATE src/sim_shell.c)
endif()
