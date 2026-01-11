#!/bin/bash
# Cannoli test runner
# Run from cannoli directory: ./t/run_tests.sh

set -e

CANNOLI_DIR="$(cd "$(dirname "$0")/.." && pwd)"
STRADA_DIR="$(cd "$CANNOLI_DIR/.." && pwd)"

# Build cannoli first
echo "Building Cannoli..."
cd "$CANNOLI_DIR"
make -s

# Run tests
echo ""
echo "Running Cannoli tests..."
echo ""

FAILED=0
PASSED=0

for test in t/test_*.strada; do
    name=$(basename "$test" .strada)
    echo "Running $name..."

    # Compile test
    if ./build_test.sh "$test" > /tmp/cannoli_test_build.log 2>&1; then
        # Run test
        if "/tmp/$name" > /tmp/cannoli_test_output.log 2>&1; then
            echo "  PASS"
            PASSED=$((PASSED + 1))
        else
            echo "  FAIL (runtime error)"
            cat /tmp/cannoli_test_output.log
            FAILED=$((FAILED + 1))
        fi
    else
        echo "  FAIL (compile error)"
        cat /tmp/cannoli_test_build.log
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "========================================"
echo "Tests: $((PASSED + FAILED))  Passed: $PASSED  Failed: $FAILED"
echo "========================================"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
