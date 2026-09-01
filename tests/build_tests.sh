#!/bin/bash

# Test build script for wheel sensor speedometer
# This script provides convenient commands for building and running tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default values
BOARD="native_sim/native/64"
BUILD_DIR="build/tests"
TEST_DIR="tests"
VERBOSE=false
CLEAN=false

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Usage function
usage() {
    echo "Usage: $0 [OPTIONS] [COMMAND]"
    echo ""
    echo "Options:"
    echo "  -b, --board BOARD    Target board (default: $BOARD)"
    echo "  -p, --pristine       Pristine build (clean first)"
    echo "  -v, --verbose        Verbose output"
    echo "  -h, --help           Show this help message"
    echo ""
    echo "Commands:"
    echo "  all                 Build and run all tests"
    echo "  build               Build all tests"
    echo "  run                 Run all tests"
    echo "  wheel_sensor        Build and run wheel sensor tests"
    echo "  battery             Build and run battery tests"
    echo "  storage             Build and run storage tests"
    echo "  sd_card             Build and run SD card tests"
    echo "  speed_calculator    Build and run speed calculator tests"
    echo "  display             Build and run display tests"
    echo "  clean               Clean test build directory"
    echo "  coverage            Run tests with coverage and generate report"
    echo "  coverage-open      Open coverage report in browser"
    echo ""
    echo "Examples:"
    echo "  $0 all                    # Build and run all tests"
    echo "  $0 -p build               # Pristine build of all tests"
    echo "  $0 wheel_sensor            # Build and run wheel sensor tests"
    echo "  $0 -b native_sim/native/64 -v run    # Run tests with verbose output"
}

# Parse options
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -p|--pristine)
            CLEAN=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            COMMAND="$1"
            shift
            ;;
    esac
done

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}[TEST]${NC} $message"
}

# Function to print error
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to print success
print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Function to print info
print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Clean function
clean() {
    print_status "$YELLOW" "Cleaning test build directory..."
    if [ -d "$PROJECT_DIR/$BUILD_DIR" ]; then
        rm -rf "$PROJECT_DIR/$BUILD_DIR"
        print_success "Cleaned test build directory"
    else
        print_info "No test build directory to clean"
    fi
}

# Build function
build() {
    local target="$1"
    local build_cmd="west build -b $BOARD -P $TEST_DIR"
    
    if [ "$target" = "all" ] || [ -z "$target" ]; then
        build_cmd="$build_cmd $PROJECT_DIR"
    else
        build_cmd="$build_cmd $PROJECT_DIR/tests/test_$target"
    fi
    
    if [ "$CLEAN" = true ]; then
        clean
    fi
    
    if [ "$VERBOSE" = true ]; then
        print_status "$YELLOW" "Building $target tests..."
        echo "Command: $build_cmd"
        $build_cmd
    else
        print_status "$YELLOW" "Building $target tests..."
        $build_cmd > /dev/null 2>&1
    fi
    
    print_success "Built $target tests"
}

# Run function
run() {
    local target="$1"
    local run_cmd="west twister -p $BOARD -P $TEST_DIR"
    
    if [ "$target" = "all" ] || [ -z "$target" ]; then
        run_cmd="$run_cmd"
    else
        run_cmd="$run_cmd -n test_$target"
    fi
    
    if [ "$VERBOSE" = true ]; then
        print_status "$YELLOW" "Running $target tests..."
        echo "Command: $run_cmd"
        $run_cmd
    else
        print_status "$YELLOW" "Running $target tests..."
        $run_cmd
    fi
}

# Build and run function
build_and_run() {
    local target="$1"
    build "$target"
    run "$target"
}

# Run with specific scenario function
run_scenario() {
    local scenario="$1"
    local run_cmd="west twister -p $BOARD -P $TEST_DIR -s $scenario"
    
    if [ "$VERBOSE" = true ]; then
        print_status "$YELLOW" "Running $scenario..."
        echo "Command: $run_cmd"
        $run_cmd
    else
        print_status "$YELLOW" "Running $scenario..."
        $run_cmd
    fi
}

# Main logic
if [ -z "$COMMAND" ]; then
    COMMAND="all"
fi

case $COMMAND in
    all)
        print_status "$YELLOW" "Building and running all tests..."
        build_and_run "all"
        ;;
    build)
        print_status "$YELLOW" "Building all tests..."
        build "all"
        ;;
    run)
        print_status "$YELLOW" "Running all tests..."
        run "all"
        ;;
    wheel_sensor)
        print_status "$YELLOW" "Building and running wheel sensor tests..."
        run_scenario "wheel_sensor_tests"
        ;;
    battery)
        print_status "$YELLOW" "Building and running battery tests..."
        run_scenario "battery_tests"
        ;;
    storage)
        print_status "$YELLOW" "Building and running storage tests..."
        run_scenario "storage_tests"
        ;;
    sd_card)
        print_status "$YELLOW" "Building and running SD card tests..."
        run_scenario "sd_card_tests"
        ;;
    speed_calculator)
        print_status "$YELLOW" "Building and running speed calculator tests..."
        run_scenario "speed_calculator_tests"
        ;;
    display)
        print_status "$YELLOW" "Building and running display tests..."
        run_scenario "display_tests"
        ;;
    clean)
        clean
        ;;
    coverage)
        print_status "$YELLOW" "Running tests with coverage..."
        build_and_run "all"
        
        # Generate coverage report
        print_status "$YELLOW" "Generating coverage report..."
        if [ -d "$PROJECT_DIR/$BUILD_DIR" ]; then
            cd "$PROJECT_DIR/$BUILD_DIR"
            if command -v gcov &> /dev/null; then
                gcov -b -c -r .
                lcov --capture --directory . --output-file coverage.info
                genhtml coverage.info --output-directory coverage_html
                print_success "Coverage report generated in $PROJECT_DIR/$BUILD_DIR/coverage_html"
            else
                print_error "gcov not found. Install gcov/lcov/genhtml for coverage reports."
            fi
            cd "$PROJECT_DIR"
        else
            print_error "Build directory not found. Run tests first."
        fi
        ;;
    coverage-open)
        # Open coverage report in browser
        COVERAGE_DIR="$PROJECT_DIR/$BUILD_DIR/coverage_html/index.html"
        if [ -f "$COVERAGE_DIR" ]; then
            if command -v xdg-open &> /dev/null; then
                xdg-open "$COVERAGE_DIR"
            elif command -v open &> /dev/null; then
                open "$COVERAGE_DIR"
            else
                print_error "No browser opener found. Open $COVERAGE_DIR manually."
            fi
        else
            print_error "Coverage report not found. Run 'coverage' command first."
        fi
        ;;
    *)
        print_error "Unknown command: $COMMAND"
        usage
        exit 1
        ;;
esac