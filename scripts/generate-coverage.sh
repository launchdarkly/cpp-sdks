#!/bin/bash

# Script to generate code coverage reports for LaunchDarkly C++ SDK
# Requirements: lcov, genhtml

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"
COVERAGE_DIR="${BUILD_DIR}/coverage"

echo "Generating code coverage report..."
echo "Build directory: $BUILD_DIR"
echo "Coverage output directory: $COVERAGE_DIR"

# Create coverage directory
mkdir -p "$COVERAGE_DIR"

# Capture coverage data
echo "Capturing coverage data..."
lcov --capture \
     --directory "$BUILD_DIR" \
     --output-file "$COVERAGE_DIR/coverage.info" \
     --rc lcov_branch_coverage=1 \
     --ignore-errors mismatch,negative

# Remove external dependencies from coverage
echo "Filtering coverage data..."
lcov --remove "$COVERAGE_DIR/coverage.info" \
     '*/build/_deps/*' \
     '*/vendor/*' \
     '*/tests/*' \
     '*/usr/*' \
     --output-file "$COVERAGE_DIR/coverage_filtered.info" \
     --rc lcov_branch_coverage=1

# Generate HTML report
echo "Generating HTML report..."
genhtml "$COVERAGE_DIR/coverage_filtered.info" \
        --output-directory "$COVERAGE_DIR/html" \
        --title "LaunchDarkly C++ SDK Coverage" \
        --legend \
        --show-details \
        --branch-coverage \
        --rc genhtml_branch_coverage=1

echo ""
echo "Coverage report generated successfully!"
echo "Open: $COVERAGE_DIR/html/index.html"
echo ""

# Print summary
lcov --summary "$COVERAGE_DIR/coverage_filtered.info" \
     --rc lcov_branch_coverage=1
