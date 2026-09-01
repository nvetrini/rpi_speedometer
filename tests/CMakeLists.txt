# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Include all test subdirectories
add_subdirectory(wheel_sensor)
add_subdirectory(battery)
add_subdirectory(storage)
add_subdirectory(sd_card)
add_subdirectory(speed_calculator)
add_subdirectory(display)