#!/bin/bash
# run.sh - Start Cannoli with the Perl example app
#
# Usage: ./run.sh [port]
#   port: Port to listen on (default: 8080)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CANNOLI_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
PORT="${1:-8080}"

# Check if Perl library is built
if [ ! -f "$CANNOLI_DIR/lib/perl/cannoli_perl.so" ]; then
    echo "Building Perl library..."
    (cd "$CANNOLI_DIR/lib/perl" && make)
fi

# Check if cannoli is built
if [ ! -f "$CANNOLI_DIR/cannoli" ]; then
    echo "Building Cannoli..."
    (cd "$CANNOLI_DIR" && make)
fi

echo "Starting Cannoli with Perl handler on port $PORT..."
echo "Try these endpoints:"
echo "  http://localhost:$PORT/           - Home page"
echo "  http://localhost:$PORT/api/users  - List users (JSON)"
echo "  http://localhost:$PORT/api/todos  - List todos (JSON)"
echo "  http://localhost:$PORT/info       - Server info"
echo ""

cd "$CANNOLI_DIR"
exec ./cannoli \
    --library "./lib/perl/cannoli_perl.so:script=$SCRIPT_DIR/app.pl;handler=MyApp::handle" \
    --dev \
    --debug \
    -p "$PORT"
