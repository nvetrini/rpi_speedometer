#!/usr/bin/env bash

set -eu

# Source the virtual environment that contains west
if [ -f "../.venv/bin/activate" ]; then
    source "../.venv/bin/activate"
fi

# Pass through any arguments to west
west build -b rpi_pico "$@"
