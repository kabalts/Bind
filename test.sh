#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$SCRIPT_DIR/build/bin"
DATA_DIR="$SCRIPT_DIR/test_data"

PASS=0
FAIL=0
TOTAL=0

section() {
    echo ""
    echo -e "${BLUE}--- $1 ---${NC}"
}

pass() {
    echo -e "  ${GREEN}[PASS]${NC} $1"
    PASS=$((PASS + 1)); TOTAL=$((TOTAL + 1))
}

fail() {
    echo -e "  ${RED}[FAIL]${NC} $1"
    FAIL=$((FAIL + 1)); TOTAL=$((TOTAL + 1))
}

has()    { echo "$2" | grep -qF "$3" && pass "$1" || fail "$1 (missing: '$3')"; }
hasnt()  { echo "$2" | grep -qF "$3" && fail "$1 (found: '$3')" || pass "$1"; }
count()  {
    local n; n=$(echo "$2" | grep -cF "$3" || true)
    [ "$n" -eq "$4" ] && pass "$1 ($4)" || fail "$1 (expected $4, got $n)"
}

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Bind Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

if [ ! -x "$BIN_DIR/bind_test" ] || [ ! -x "$BIN_DIR/bind_server" ] || [ ! -x "$BIN_DIR/bind_client" ]; then
    echo -e "${RED}ERROR: Binaries not found. Run 'bash build.sh' first.${NC}"
    exit 1
fi

# ============================================================
section "Phase 1: Unit Tests"
# ============================================================

UNIT=$("$BIN_DIR/bind_test" 2>&1)
echo "$UNIT" | tail -n3

if echo "$UNIT" | grep -q "0 failed"; then
    pass "All C++ unit tests"
else
    fail "Some C++ unit tests failed"
fi

# ============================================================
section "Phase 2: SQL Integration Tests"
# ============================================================

rm -rf "$DATA_DIR"

echo -n "Starting server ... "
"$BIN_DIR/bind_server" -p 9888 -d "$DATA_DIR" > /dev/null 2>&1 &
SVR=$!
sleep 1
if ! kill -0 "$SVR" 2>/dev/null; then
    echo -e "${RED}FAILED${NC}"
    exit 1
fi
echo "OK (pid=$SVR)"

cleanup() {
    echo ""
    echo "Shutting down ..."
    kill "$SVR" 2>/dev/null; wait "$SVR" 2>/dev/null
    rm -rf "$DATA_DIR"
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  ${GREEN}$PASS passed${NC}  ${RED}$FAIL failed${NC}  $TOTAL total"
    echo -e "${BLUE}========================================${NC}"
    [ "$FAIL" -gt 0 ] && exit 1
}
trap cleanup EXIT INT TERM

OUT=$(cat "$SCRIPT_DIR/test_samples.sql" | "$BIN_DIR/bind_client" -p 9888 2>&1)
echo "$OUT"

# ============================================================
section "2.1 DDL"
# ============================================================
has    "create database"    "$OUT" "Database 'testdb' created."
has    "use database"       "$OUT" "Using database 'testdb'."
has    "use nonexist fails" "$OUT" "ERROR: Failed to open database"
has    "create table"       "$OUT" "Table 'person' created."
has    "drop table"         "$OUT" "Table 'person' dropped."
has    "drop database"      "$OUT" "Database 'testdb' dropped."

# ============================================================
section "2.2 INSERT & Primary Key"
# ============================================================
count  "4 successful INSERTs"  "$OUT" "1 row inserted."  4
count  "2 PK rejections"      "$OUT" "ERROR: Insert failed" 2
hasnt  "DuplicatePK not stored" "$OUT" "DuplicatePK"
hasnt  "AlsoDup not stored"     "$OUT" "AlsoDup"
has    "Alice in results"       "$OUT" "Alice"

# ============================================================
section "2.3 SELECT"
# ============================================================
has    "Bob in results"        "$OUT" "Bob"
has    "Charlie in results"    "$OUT" "Charlie"
has    "3 rows from SELECT *"  "$OUT" "3 rows in set"

# ============================================================
section "2.4 UPDATE"
# ============================================================
count  "1 successful UPDATE"         "$OUT" "Rows updated." 1
has    "Alice2 after update"         "$OUT" "Alice2"

# ============================================================
section "2.5 DELETE"
# ============================================================
count  "2 successful DELETEs"        "$OUT" "Rows deleted." 2
count  "2 no-match errors (update+delete)" "$OUT" "ERROR: No matching row found" 2
has    "AliceReinserted after delete" "$OUT" "AliceReinserted"

# ============================================================
section "2.6 Final state"
# ============================================================
has    "exit message" "$OUT" "Bye"
