# Live Device Test Results -- iPad8,10 iOS 26.3
# Date: 2026-04-05

## Device
- Model: iPad8,10 (iPad Pro A12X/A12Z)
- iOS: 26.3
- UDID: 00008027-001C484C3A62802E
- ECID: 7960791582146606
- ChipID: 0x8027 (A12Z)
- Serial: DMPC20KDPV13
- IMEI: 356622100389197
- WiFi: 3c:bf:60:65:2d:27
- BT: 3c:bf:60:65:3c:1c

## Command Enumeration on iOS 26.3 mobileactivationd

### VALID Commands:
- GetActivationStateRequest -> returns "Unactivated"
- CreateTunnel1SessionInfoRequest -> returns CollectionBlob, HandshakeRequestMessage, UDID
- HandleActivationInfoRequest -> accepts Value, attempts activation (needs valid record)
- HandleActivationInfoWithSessionRequest -> accepts Value + headers (needs valid nonce)
- CreateActivationInfoRequest -> exists but fails (needs DRM session)

### REMOVED Commands (present in older iOS, gone in 26.3):
- CreateTunnel1FactoryActivationInfoRequest -> UNKNOWN
- CreateFactoryActivationInfoRequest -> UNKNOWN
- HandleFactoryActivationInfoRequest -> UNKNOWN
- FactoryActivateRequest -> UNKNOWN
- DeactivateRequest -> UNKNOWN

### KEY FINDING: Apple removed ALL factory activation commands on iOS 26.3
This is what changed between 26.1 (bypassable) and 26.2+ (not bypassable).
Checkm8.info and iRemoveTools used the factory activation path, which no
longer exists on iOS 26.3.

## Activation Protocol Capture

### Session Info (from device):
- CollectionBlob: ~26KB XML plist containing:
  - IngestBody: JSON with serial, IMEI, UDID, productType, osVersion
  - scrt-part2: Cryptographic blob (~900 bytes)
  - pcrt: Cryptographic blob (~1100 bytes)
  - X-Apple-Sig-Key: ECDSA public key (base64)
  - X-Apple-Signature: ECDSA signature of IngestBody (base64)
- HandshakeRequestMessage: 21 bytes (DRM session initiation)
- UniqueDeviceID: UDID string

### drmHandshake (Apple's real response):
- HTTP 200 from albert.apple.com
- Returns: serverKP, FDRBlob, SUInfo, HandshakeResponseMessage
- 1340 bytes total

### Activation Attempt Results:
| Method | Value Format | Error |
|--------|-------------|-------|
| HandleActivationInfoRequest | dict directly | Code -2: "Failed to activate device" |
| HandleActivationInfoRequest | {activation-record: ...} | Code -1: "Invalid activation record" |
| HandleActivationInfoWithSessionRequest | + headers | Code -1: "Invalid activation nonce" |
| activate_with_session (pymobiledevice3) | XML record | Accepted (no error) but state unchanged |
| set_value ActivationState | "Activated" | Accepted (no error) but state unchanged |

### "Invalid activation nonce" Error
When using HandleActivationInfoWithSessionRequest, the device validates
the activation nonce in the record. The nonce is derived from the DRM
session (FairPlay). Without a valid DRM session (blocked by iCloud lock),
we cannot generate the correct nonce.

## Barriers Confirmed by Live Testing

1. **Factory activation path REMOVED on iOS 26.3** -- this is THE change
   that blocks existing tools. Commands simply don't exist anymore.

2. **DRM session refuses to establish** -- create_activation_info fails with
   "Failed to establish session / Invalid input" because the device's
   FairPlay engine checks iCloud lock LOCALLY before generating the nonce.

3. **Activation nonce required** -- even bypassing the DRM session and
   submitting records directly, the device validates the nonce. Without
   the session, no valid nonce exists.

4. **Apple's server returns buddyml** -- even with valid protocol, Apple's
   server returns an iCloud login form instead of activation data.

## What Still Might Work (Not Yet Tested)

1. **Captive portal / WebSheet approach** -- hasn't been tested yet. This
   bypasses both the DRM session AND Apple's server by injecting directly
   into the activation flow during Setup Assistant via WiFi.

2. **VU#346053 XML injection** -- targets a different endpoint (humbug/baa)
   that may not require the DRM nonce.

3. **WebSheet profile installation** -- install Root CA + MDM enrollment
   via captive portal, then MITM activation from the device's perspective
   (the device initiates activation, not our tool).

4. **Nonce extraction from CollectionBlob** -- the scrt-part2 and pcrt
   blobs contain cryptographic material. Analyzing them might reveal
   how to derive the activation nonce without the DRM session.

5. **Replay a valid activation** -- if we could capture a successful
   activation from another device, the nonce format and structure could
   inform how to construct one.
