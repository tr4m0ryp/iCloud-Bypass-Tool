#!/usr/bin/env bash
#
# test_local.sh -- Test all captive portal endpoints on localhost.
#
# This script starts the portal server on a non-privileged port (8080),
# runs through every endpoint, then cleans up.
#
# Usage:
#   chmod +x test_local.sh
#   ./test_local.sh
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SERVER_SCRIPT="$SCRIPT_DIR/server/portal.py"
PORT=8080
BASE="http://localhost:$PORT"
PASS=0
FAIL=0

# -- Helpers --------------------------------------------------------------

print_header() {
    echo ""
    echo "============================================"
    echo "  $1"
    echo "============================================"
}

check_response() {
    local label="$1"
    local expected="$2"
    local actual="$3"

    if echo "$actual" | grep -q "$expected"; then
        echo "  [PASS] $label"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $label -- expected '$expected' in response"
        echo "  Response: $(echo "$actual" | head -5)"
        FAIL=$((FAIL + 1))
    fi
}

cleanup() {
    if [ -n "${SERVER_PID:-}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# -- Start server on non-privileged port ---------------------------------

print_header "Starting server on port $PORT"

# Start server with env var port override (portal.py reads PORTAL_PORT)
PORTAL_PORT=$PORT python3 "$SERVER_SCRIPT" &
SERVER_PID=$!

# Wait for server to start
sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Error: Server failed to start."
    exit 1
fi
echo "  Server PID: $SERVER_PID"

# -- Test GET endpoints ---------------------------------------------------

print_header "GET / (status page)"
RESP=$(curl -s "$BASE/")
check_response "Status page returns HTML" "Captive Portal Server" "$RESP"

print_header "GET /hotspot-detect.html (captive detection)"
RESP=$(curl -s "$BASE/hotspot-detect.html")
check_response "Does NOT return Success (triggers WebSheet)" "Network Configuration Required" "$RESP"
# Verify it does NOT contain the Apple success string
if echo "$RESP" | grep -q "<TITLE>Success</TITLE>"; then
    echo "  [FAIL] Response contains Success -- WebSheet would dismiss"
    FAIL=$((FAIL + 1))
else
    echo "  [PASS] No Success title -- WebSheet stays open"
    PASS=$((PASS + 1))
fi

print_header "GET /captive-portal (portal page)"
RESP=$(curl -s "$BASE/captive-portal")
check_response "Portal page has profile link" "profile.mobileconfig" "$RESP"

print_header "GET /profile.mobileconfig"
RESP=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/profile.mobileconfig")
# Profile may not exist yet (404 is acceptable if not built)
if [ "$RESP" = "200" ]; then
    echo "  [PASS] Profile served (200)"
    PASS=$((PASS + 1))
elif [ "$RESP" = "404" ]; then
    echo "  [PASS] Profile not built yet (404 expected before build_profile.py)"
    PASS=$((PASS + 1))
else
    echo "  [FAIL] Unexpected status: $RESP"
    FAIL=$((FAIL + 1))
fi

# -- Test POST endpoints --------------------------------------------------

print_header "POST /deviceservices/drmHandshake"
RESP=$(curl -s -X POST \
    -H "Content-Type: application/x-apple-plist" \
    -H "User-Agent: iOS Device Activator (MobileActivation-592.103.2)" \
    -d '<?xml version="1.0"?><plist version="1.0"><dict><key>Test</key><true/></dict></plist>' \
    "$BASE/deviceservices/drmHandshake")
check_response "Handshake returns plist" "HandshakeResponseMessage" "$RESP"

print_header "POST /deviceservices/deviceActivation"
RESP=$(curl -s -X POST \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -H "User-Agent: iOS Device Activator (MobileActivation-592.103.2)" \
    -d 'activation-info=test_activation_data' \
    "$BASE/deviceservices/deviceActivation")
check_response "Activation returns record" "ActivationRecord" "$RESP"

print_header "POST /humbug/baa"
RESP=$(curl -s -X POST \
    -H "Content-Type: application/x-apple-plist" \
    -H "User-Agent: iOS Device Activator (MobileActivation-592.103.2)" \
    -d '<?xml version="1.0"?><plist version="1.0"><dict><key>Nonce</key><data>AAAA</data></dict></plist>' \
    "$BASE/humbug/baa")
check_response "BAA returns provisioning response" "ProvisioningResponse" "$RESP"

# -- Test 404 handling ----------------------------------------------------

print_header "GET /nonexistent (404)"
RESP=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/nonexistent")
check_response "Returns 404" "404" "$RESP"

# -- Summary --------------------------------------------------------------

print_header "Results"
TOTAL=$((PASS + FAIL))
echo "  Passed: $PASS / $TOTAL"
echo "  Failed: $FAIL / $TOTAL"

if [ "$FAIL" -gt 0 ]; then
    echo ""
    echo "  Some tests failed."
    exit 1
else
    echo ""
    echo "  All tests passed."
    exit 0
fi
