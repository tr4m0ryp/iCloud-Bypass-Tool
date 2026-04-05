#!/usr/bin/env python3
"""
Activation protocol response builders.

Constructs plist responses that mimic Apple's activation servers:
  - albert.apple.com  /deviceservices/drmHandshake
  - albert.apple.com  /deviceservices/deviceActivation
  - humb.apple.com    /humbug/baa

All certificate/key data uses placeholder values marked with TODO.
The plist structure is derived from binary analysis of MobileActivation.
"""

import plistlib
import uuid
import time
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Constants derived from MobileActivation binary analysis
# ---------------------------------------------------------------------------
ACTIVATION_USER_AGENT = "iOS Device Activator (MobileActivation-592.103.2)"
HOST_IDENTIFIER = "thephonedoesntcarewhatisendhereitseems"

# TODO: Replace these with real extracted/generated values.
PLACEHOLDER_CERT = (
    b"-----BEGIN CERTIFICATE-----\n"
    b"PLACEHOLDER_DEVICE_CERTIFICATE_DATA\n"
    b"TODO: Replace with actual device certificate\n"
    b"-----END CERTIFICATE-----\n"
)

PLACEHOLDER_KEY_DATA = (
    b"PLACEHOLDER_FAIRPLAY_KEY_DATA_"
    b"TODO_REPLACE_WITH_ACTUAL_FP_KEYS"
)

PLACEHOLDER_FDR_BLOB = (
    b"PLACEHOLDER_FDR_BLOB_DATA_"
    b"TODO_REPLACE_WITH_EXTRACTED_FDR"
)

PLACEHOLDER_SERVER_KP = (
    b"PLACEHOLDER_SERVER_KP_DATA_"
    b"TODO_REPLACE_WITH_SERVER_KEYPAIR"
)

PLACEHOLDER_ACCOUNT_TOKEN = (
    b"PLACEHOLDER_ACCOUNT_TOKEN_"
    b"TODO_REPLACE_WITH_VALID_TOKEN"
)


# ---------------------------------------------------------------------------
# drmHandshake response
# ---------------------------------------------------------------------------
def build_handshake_response():
    """
    Build a drmHandshake response plist.

    This is the first message in the activation handshake.  The device
    sends its FairPlay identity; the server replies with keying material
    that the device uses to build the full activation request.

    Returns:
        bytes: Binary plist data.
    """
    response = {
        "HandshakeResponseMessage": {
            # Server key-pair material for the DRM session
            # TODO: These must match the FairPlay protocol expectations
            "serverKP": PLACEHOLDER_SERVER_KP,

            # Factory Data Reset blob -- device uses this to prove identity
            "FDRBlob": PLACEHOLDER_FDR_BLOB,

            # Software Update info block
            "SUInfo": {
                "SUDocumentID": str(uuid.uuid4()),
                "SUAllowedIn": True,
                "SUVersion": "1.0",
            },

            # Session identifier
            "sessionID": str(uuid.uuid4()),

            # Timestamp
            "timestamp": int(time.time()),
        },
        "Status": "SUCCESS",
        "ProtocolVersion": "2",
    }

    return plistlib.dumps(response, fmt=plistlib.FMT_XML)


# ---------------------------------------------------------------------------
# deviceActivation response
# ---------------------------------------------------------------------------
def build_activation_record(device_info):
    """
    Build a factory activation record plist.

    Mimics the response from createFActivationInfo / deviceActivation.
    The device validates this record to determine activation state.

    Args:
        device_info: Raw activation-info payload from the device (for logging).

    Returns:
        bytes: Binary plist data containing the activation record.
    """
    # Unique identifiers for this activation
    activation_id = str(uuid.uuid4())
    now_iso = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    activation_record = {
        # Account binding token
        # TODO: Must be a valid token that passes MobileActivation validation
        "AccountToken": PLACEHOLDER_ACCOUNT_TOKEN,

        # Device certificate chain
        # TODO: Must match device's FairPlay identity
        "DeviceCertificate": PLACEHOLDER_CERT,

        # FairPlay key data for DRM binding
        # TODO: Derive from handshake session
        "FairPlayKeyData": PLACEHOLDER_KEY_DATA,

        # Activation state
        "ActivationState": "Activated",
        "ActivationStateDescription": "Factory Activated",

        # Record metadata
        "ActivationRecordID": activation_id,
        "ActivationTimestamp": now_iso,

        # Brick state flag -- False means the device should boot normally
        "BrickState": False,

        # Push registration (for APNs)
        "PushToken": b"PLACEHOLDER_PUSH_TOKEN_TODO",

        # Provisioning info
        "UniqueDeviceID": HOST_IDENTIFIER,
    }

    # Wrap in the expected outer structure
    response = {
        "ActivationRecord": activation_record,
        "Status": "SUCCESS",
        "ProtocolVersion": "2",
        "AccountTokenCertificate": PLACEHOLDER_CERT,
        "DeviceActivation": {
            "activation-record": plistlib.dumps(
                activation_record, fmt=plistlib.FMT_XML
            ),
        },
    }

    return plistlib.dumps(response, fmt=plistlib.FMT_XML)


# ---------------------------------------------------------------------------
# humbug/baa response (VU#346053)
# ---------------------------------------------------------------------------
def build_baa_response():
    """
    Build a humb.apple.com/humbug/baa provisioning response.

    VU#346053: The baa (Brick Activation Avoidance) endpoint handles
    device provisioning.  A crafted response here can influence the
    device's activation state machine.

    Returns:
        bytes: Binary plist data.
    """
    response = {
        "ProtocolVersion": "2",
        "Status": "SUCCESS",

        # Provisioning response body
        "ProvisioningResponse": {
            # TODO: The real response includes signed provisioning data
            "ProvisioningCertificateChain": PLACEHOLDER_CERT,
            "ProvisioningData": (
                b"PLACEHOLDER_PROVISIONING_DATA_"
                b"TODO_REPLACE_WITH_CRAFTED_PAYLOAD"
            ),

            # Nonce echo -- the device checks this matches its request
            "Nonce": b"PLACEHOLDER_NONCE_TODO",

            # Timestamp for replay protection
            "Timestamp": int(time.time()),

            # Session binding
            "SessionID": str(uuid.uuid4()),
        },

        # BAA-specific flags
        "BAAResponse": {
            "AllowActivation": True,
            "BrickState": False,
            "GracePeriod": 0,
            "Message": "Device provisioning successful",
        },
    }

    return plistlib.dumps(response, fmt=plistlib.FMT_XML)
