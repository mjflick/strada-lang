#!/bin/bash
#
# Cannoli Stress Test
# Tests server stability under load
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CANNOLI_DIR="$(dirname "$SCRIPT_DIR")"
STRADA_DIR="$(dirname "$CANNOLI_DIR")"
STATIC_DIR="$STRADA_DIR/website"

# Configuration
PORT=8099
REQUESTS=${1:-1000}
CONCURRENT=${2:-1}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "============================================"
echo "Cannoli Stress Test"
echo "============================================"
echo "Requests: $REQUESTS"
echo "Concurrent: $CONCURRENT"
echo "Port: $PORT"
echo "Static dir: $STATIC_DIR"
echo ""

# Clean up any existing cannoli processes on our port
pkill -9 -f "cannoli.*$PORT" 2>/dev/null || true
sleep 1

# Build cannoli if needed
if [ ! -f "$CANNOLI_DIR/cannoli" ]; then
    echo "Building cannoli..."
    cd "$CANNOLI_DIR"
    "$STRADA_DIR/strada" build/cannoli.strada
fi

# Start server
echo "Starting cannoli server..."
cd "$CANNOLI_DIR"
./cannoli --dev --port $PORT --listing --static "$STATIC_DIR" > /tmp/cannoli_stress.log 2>&1 &
SERVER_PID=$!
sleep 2

# Check if server started
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}FAIL: Server failed to start${NC}"
    cat /tmp/cannoli_stress.log
    exit 1
fi

echo -e "${GREEN}Server started (PID: $SERVER_PID)${NC}"
echo ""

# Test endpoints
ENDPOINTS=(
    "/"
    "/docs/"
    "/docs/index.html"
    "/docs/basics.html"
    "/docs/types.html"
    "/docs/functions.html"
    "/docs/control-flow.html"
    "/docs/oop.html"
    "/docs/reference.html"
    "/docs/style.css"
)

FAILED=0
SUCCEEDED=0

run_test() {
    local name="$1"
    local count="$2"

    echo -n "Test: $name ($count requests)... "

    local start_time=$(date +%s.%N)

    for i in $(seq 1 $count); do
        # Pick a random endpoint
        local idx=$((RANDOM % ${#ENDPOINTS[@]}))
        local endpoint="${ENDPOINTS[$idx]}"

        if ! curl -s -f "http://localhost:$PORT$endpoint" > /dev/null 2>&1; then
            echo -e "${RED}FAIL${NC} (request $i to $endpoint)"
            return 1
        fi

        # Check server is still running every 100 requests
        if [ $((i % 100)) -eq 0 ]; then
            if ! kill -0 $SERVER_PID 2>/dev/null; then
                echo -e "${RED}FAIL${NC} (server crashed at request $i)"
                return 1
            fi
        fi
    done

    local end_time=$(date +%s.%N)
    local elapsed=$(echo "$end_time - $start_time" | bc)
    local rps=$(echo "scale=1; $count / $elapsed" | bc)

    # Final check
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}FAIL${NC} (server crashed after completing)"
        return 1
    fi

    echo -e "${GREEN}OK${NC} (${elapsed}s, ${rps} req/s)"
    return 0
}

echo "============================================"
echo "Running stress tests..."
echo "============================================"
echo ""

# Test 1: Sequential requests
if run_test "Sequential requests" $REQUESTS; then
    SUCCEEDED=$((SUCCEEDED + 1))
else
    FAILED=$((FAILED + 1))
fi

# Test 2: Rapid-fire to same endpoint
echo -n "Test: Rapid-fire same endpoint (500 requests)... "
for i in $(seq 1 500); do
    curl -s "http://localhost:$PORT/" > /dev/null 2>&1
done
if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
    SUCCEEDED=$((SUCCEEDED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 3: 404 handling
echo -n "Test: 404 handling (100 requests)... "
for i in $(seq 1 100); do
    curl -s "http://localhost:$PORT/nonexistent$i.html" > /dev/null 2>&1
done
if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
    SUCCEEDED=$((SUCCEEDED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 4: Concurrent requests (if curl supports it or use background)
if [ "$CONCURRENT" -gt 1 ]; then
    echo -n "Test: Concurrent requests ($CONCURRENT parallel, 100 batches)... "
    for batch in $(seq 1 100); do
        for c in $(seq 1 $CONCURRENT); do
            curl -s "http://localhost:$PORT/" > /dev/null 2>&1 &
        done
        wait
    done
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${GREEN}OK${NC}"
        SUCCEEDED=$((SUCCEEDED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
    fi
fi

# Test 5: Large file (if exists)
if [ -f "$STATIC_DIR/docs/reference.html" ]; then
    echo -n "Test: Large file requests (200 requests)... "
    for i in $(seq 1 200); do
        curl -s "http://localhost:$PORT/docs/reference.html" > /dev/null 2>&1
    done
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${GREEN}OK${NC}"
        SUCCEEDED=$((SUCCEEDED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
    fi
fi

# Test 6: Directory listing
echo -n "Test: Directory listing (100 requests)... "
for i in $(seq 1 100); do
    curl -s "http://localhost:$PORT/docs/" > /dev/null 2>&1
done
if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${GREEN}OK${NC}"
    SUCCEEDED=$((SUCCEEDED + 1))
else
    echo -e "${RED}FAIL${NC}"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "============================================"
echo "Results"
echo "============================================"

# Cleanup
kill $SERVER_PID 2>/dev/null || true

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All $SUCCEEDED tests passed!${NC}"
    echo ""
    echo "Server handled $((REQUESTS + 500 + 100 + 200 + 100))+ requests without crashing."
    exit 0
else
    echo -e "${RED}$FAILED tests failed, $SUCCEEDED passed${NC}"
    echo ""
    echo "Check /tmp/cannoli_stress.log for server output"
    exit 1
fi
