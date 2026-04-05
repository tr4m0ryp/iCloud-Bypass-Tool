#!/bin/bash
# Itr4m -- Guided activation lock bypass wrapper
# Detects device mode, guides DFU entry if needed, runs itr4m binary.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="$SCRIPT_DIR/itr4m"

# ------------------------------------------------------------------ #
# Banner                                                               #
# ------------------------------------------------------------------ #

print_banner() {
    echo "========================================"
    echo "  Itr4m v0.1.0"
    echo "  Activation lock bypass research tool"
    echo "========================================"
    echo ""
}

# ------------------------------------------------------------------ #
# Dependency checks                                                    #
# ------------------------------------------------------------------ #

check_deps() {
    if [ ! -x "$BINARY" ]; then
        echo "[-] Binary not found: $BINARY"
        echo "    Run 'make' first to build the project."
        exit 1
    fi
}

# ------------------------------------------------------------------ #
# Device detection                                                     #
# ------------------------------------------------------------------ #

check_dfu() {
    # Look for Apple DFU device (PID 0x1227) in system USB data
    if system_profiler SPUSBDataType 2>/dev/null | grep -q "0x1227"; then
        return 0
    fi
    return 1
}

check_normal() {
    # Check if libimobiledevice can see a device
    if command -v idevice_id >/dev/null 2>&1; then
        local devices
        devices="$(idevice_id -l 2>/dev/null || true)"
        if [ -n "$devices" ]; then
            return 0
        fi
    fi
    return 1
}

# ------------------------------------------------------------------ #
# DFU mode guide                                                       #
# ------------------------------------------------------------------ #

guide_dfu() {
    echo "[*] No device detected in normal or DFU mode."
    echo ""
    echo "To enter DFU mode:"
    echo ""
    echo "  For iPhone 7 / iPad (Home button):"
    echo "    1. Connect device to Mac via USB"
    echo "    2. Power off the device completely"
    echo "    3. Hold Power + Home for 10 seconds"
    echo "    4. Release Power, keep holding Home for 5 more seconds"
    echo "    5. Screen must be BLACK (not Apple logo)"
    echo ""
    echo "  For iPhone 8+ / iPad Pro (Face ID):"
    echo "    1. Connect device to Mac via USB"
    echo "    2. Quick-press Volume Up, then Volume Down"
    echo "    3. Hold Side button until screen goes black"
    echo "    4. Hold Side + Volume Down for 5 seconds"
    echo "    5. Release Side, keep holding Volume Down for 10 seconds"
    echo "    6. Screen must be BLACK (not Apple logo)"
    echo ""
    echo "If you see the Apple logo, you are in recovery mode (not DFU)."
    echo "Start over from step 2."
    echo ""
    echo "Press Enter when the device is in DFU mode..."
    read -r
}

# ------------------------------------------------------------------ #
# Main                                                                 #
# ------------------------------------------------------------------ #

print_banner
check_deps

if check_dfu; then
    echo "[+] Device detected in DFU mode"
elif check_normal; then
    echo "[+] Device detected in normal mode"
else
    guide_dfu

    # Re-check after user claims DFU entry
    if check_dfu; then
        echo "[+] Device detected in DFU mode"
    else
        echo "[-] Device not detected in DFU mode."
        echo "    Verify the USB connection and try again."
        exit 1
    fi
fi

echo ""
echo "[*] Starting itr4m..."
echo ""

exec "$BINARY" "$@"
