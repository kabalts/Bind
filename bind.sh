#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$SCRIPT_DIR/build/bin"

if [ ! -x "$BIN_DIR/bind_server" ] || [ ! -x "$BIN_DIR/bind_client" ]; then
    echo "Binary not found, building first..."
    "$SCRIPT_DIR/build.sh"
    echo ""
fi

echo "Starting Bind Server (port 8888) ..."
"$BIN_DIR/bind_server" > /dev/null 2>&1 &
SERVER_PID=$!

sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: Server failed to start."
    exit 1
fi

cleanup() {
    echo ""
    echo "Shutting down server..."
    kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null
    echo "Done."
}
trap cleanup EXIT INT TERM

echo "Starting Bind Client ..."
echo ""
"$BIN_DIR/bind_client"
