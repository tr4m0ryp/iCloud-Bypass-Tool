#!/usr/bin/env python3
"""
Build a .mobileconfig profile that installs our Root CA certificate.

The profile is served by the captive portal to devices connecting to
the test network.  Once installed, the device trusts our CA, allowing
the server to present valid TLS certificates for Apple domains.

Usage:
    python3 build_profile.py [ca_cert_path] [output_path]

Defaults:
    ca_cert_path = ../certs/ca.crt
    output_path  = network.mobileconfig  (in this directory)
"""

import os
import sys
import uuid
import plistlib

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CA_PATH = os.path.join(SCRIPT_DIR, "..", "certs", "ca.crt")
DEFAULT_OUTPUT = os.path.join(SCRIPT_DIR, "network.mobileconfig")

# ---------------------------------------------------------------------------
# Profile identifiers
# ---------------------------------------------------------------------------
ORG_PREFIX = "com.wifi.auth"
PROFILE_DISPLAY_NAME = "Network Authentication"
PROFILE_DESCRIPTION = (
    "Installs a trusted root certificate for secure network access. "
    "This profile is required to authenticate with this network."
)


def build_root_ca_payload(ca_cert_data):
    """
    Build the Root CA trust payload.

    Args:
        ca_cert_data: Raw bytes of the CA certificate (PEM or DER).

    Returns:
        dict: A PayloadContent entry for com.apple.security.root.
    """
    return {
        "PayloadType": "com.apple.security.root",
        "PayloadVersion": 1,
        "PayloadIdentifier": f"{ORG_PREFIX}.ca",
        "PayloadUUID": str(uuid.uuid4()),
        "PayloadDisplayName": "Network Security CA",
        "PayloadDescription": "Root certificate for network authentication.",
        "PayloadContent": ca_cert_data,
        "PayloadEnabled": True,
    }


def build_wifi_payload():
    """
    Build an optional WiFi configuration payload.

    This auto-joins the device to our test network after profile install.
    Adjust SSID and security settings for your test environment.

    Returns:
        dict: A PayloadContent entry for com.apple.wifi.managed.
    """
    # TODO: Update SSID_STR to match your test access point
    return {
        "PayloadType": "com.apple.wifi.managed",
        "PayloadVersion": 1,
        "PayloadIdentifier": f"{ORG_PREFIX}.wifi",
        "PayloadUUID": str(uuid.uuid4()),
        "PayloadDisplayName": "Test Network WiFi",
        "PayloadDescription": "Auto-join configuration for the test network.",
        "PayloadEnabled": True,
        "SSID_STR": "ActivationTest",  # TODO: Change to your SSID
        "EncryptionType": "WPA2",
        "AutoJoin": True,
        "IsHotspot": False,
        "HIDDEN_NETWORK": False,
        # TODO: Add password if using WPA2-PSK
        # "Password": "testpassword",
    }


def build_profile(ca_cert_data, include_wifi=False):
    """
    Assemble the complete .mobileconfig profile.

    Args:
        ca_cert_data:  Raw CA certificate bytes.
        include_wifi:  Whether to include WiFi auto-join payload.

    Returns:
        bytes: Plist-encoded profile data.
    """
    payloads = [build_root_ca_payload(ca_cert_data)]

    if include_wifi:
        payloads.append(build_wifi_payload())

    profile = {
        "PayloadContent": payloads,
        "PayloadDisplayName": PROFILE_DISPLAY_NAME,
        "PayloadDescription": PROFILE_DESCRIPTION,
        "PayloadIdentifier": f"{ORG_PREFIX}.profile",
        "PayloadOrganization": "WiFi Auth",
        "PayloadType": "Configuration",
        "PayloadUUID": str(uuid.uuid4()),
        "PayloadVersion": 1,
        "PayloadRemovalDisallowed": False,
    }

    return plistlib.dumps(profile, fmt=plistlib.FMT_XML)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    ca_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_CA_PATH
    output_path = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUTPUT

    # Read CA certificate
    if not os.path.isfile(ca_path):
        print(f"Error: CA certificate not found at {ca_path}")
        print("Run certs/generate_ca.sh first.")
        sys.exit(1)

    with open(ca_path, "rb") as f:
        ca_data = f.read()

    print(f"CA certificate: {ca_path} ({len(ca_data)} bytes)")

    # Build profile
    profile_data = build_profile(ca_data, include_wifi=False)

    # Write output
    with open(output_path, "wb") as f:
        f.write(profile_data)

    print(f"Profile written: {output_path} ({len(profile_data)} bytes)")
    print("Payloads: Root CA trust")
    print("")
    print("The portal server will serve this file at /profile.mobileconfig")


if __name__ == "__main__":
    main()
