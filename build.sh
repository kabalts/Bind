#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

find_compiler() {
    if [ -n "$CXX" ]; then
        echo "$CXX"
    elif command -v g++-14 &>/dev/null; then
        echo "g++-14"
    elif command -v g++-13 &>/dev/null; then
        echo "g++-13"
    elif command -v g++ &>/dev/null; then
        local gcc_ver
        gcc_ver=$(g++ -dumpversion | cut -d. -f1)
        if [ "$gcc_ver" -ge 14 ]; then
            echo "g++"
        else
            echo ""
        fi
    else
        echo ""
    fi
}

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Bind - Build${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

COMPILER=$(find_compiler)
if [ -z "$COMPILER" ]; then
    echo -e "${RED}ERROR: No suitable compiler found (need g++ >= 14).${NC}"
    exit 1
fi
echo "Compiler: $COMPILER ($($COMPILER --version | head -n1))"
echo ""

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[1/2] Configuring ..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$COMPILER"

echo ""
echo "[2/2] Building ..."
make -j"$(nproc)"
BUILD_EXIT=$?

echo ""
if [ "$BUILD_EXIT" -eq 0 ]; then
    echo -e "${GREEN}=== Build OK ===${NC}"
else
    echo -e "${RED}=== Build FAILED ===${NC}"
    exit 1
fi
