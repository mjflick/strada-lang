#!/bin/bash
# Build a Cannoli test file
# Usage: ./build_test.sh t/test_config.strada

set -e

CANNOLI_DIR="$(cd "$(dirname "$0")" && pwd)"
STRADA_DIR="$(cd "$CANNOLI_DIR/.." && pwd)"
STRADAC="$STRADA_DIR/stradac"

if [ -z "$1" ]; then
    echo "Usage: $0 <test.strada>"
    exit 1
fi

TEST_FILE="$1"
TEST_NAME=$(basename "$TEST_FILE" .strada)

# Concatenate all source files with the test
COMBINED="/tmp/${TEST_NAME}_combined.strada"

# cannoli_obj.strada must come last because it declares 'package Cannoli;'
cat "$CANNOLI_DIR/src/config.strada" \
    "$CANNOLI_DIR/src/request.strada" \
    "$CANNOLI_DIR/src/response.strada" \
    "$CANNOLI_DIR/src/router.strada" \
    "$TEST_FILE" \
    "$CANNOLI_DIR/src/cannoli_obj.strada" > "$COMBINED"

# Compile
"$STRADAC" "$COMBINED" "/tmp/${TEST_NAME}.c"

# Build
gcc -o "/tmp/$TEST_NAME" "/tmp/${TEST_NAME}.c" \
    "$STRADA_DIR/runtime/strada_runtime.c" \
    -I"$STRADA_DIR/runtime" \
    -ldl -lm -lpthread

echo "Built /tmp/$TEST_NAME"
