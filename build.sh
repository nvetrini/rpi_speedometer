#!/usr/bin/env bash

set -eu

BOARD=$1
shift

# Source the virtual environment that contains west
if [ -f "../.venv/bin/activate" ]; then
    source "../.venv/bin/activate"
fi

# Pass through any arguments to west
west build -b "${BOARD}" "$@"
