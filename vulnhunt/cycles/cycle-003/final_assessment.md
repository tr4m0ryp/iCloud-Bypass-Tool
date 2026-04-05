# iOS 26.3 Activation Lock Bypass -- Final Assessment
# Date: 2026-04-05
# Device: iPad8,10 (A12Z) iOS 26.3

## Every Vector Tested

### USB-Based (via pymobiledevice3 + libimobiledevice)
| Attempt | Result |
|---------|--------|
| Set ActivationState via lockdownd | Accepted but ignored |
| HandleActivationInfoRequest with crafted record | Code -1: "Invalid activation nonce" |
| HandleActivationInfoWithSessionRequest + headers | Code -1: "Invalid activation nonce" |
| activate_with_session (pymobiledevice3) | Accepted silently, state unchanged |
| CreateActivationInfoRequest (non-session) | Fails: "Failed to establish session" |
| CreateTunnel1FactoryActivationInfoRequest | UNKNOWN command (REMOVED on 26.3) |
| lockdownd ActivationInfo key query | MissingValue |
| drmHandshake with real Apple + device session | Apple: 200 OK. Device: "Invalid input" (DRM locked) |
| Full proxy through Apple (handshake + activation) | Apple returns buddyml (login form) |

### AFC Filesystem (read/write access confirmed)
| Attempt | Result |
|---------|--------|
| Write SkipSetup to /iTunes_Control/iTunes/ | Written, ignored after reboot |
| Write SetupDone to /iTunes_Control/iTunes/ | Written, ignored after reboot |
| AFC path traversal (../, %2F, absolute paths) | Blocked (status 7/8) |
| Symlink/hardlink creation | API not available |
| Write crafted downloads.28.sqlitedb (bookassetd) | Written, no processing on 26.3 |
| Write BLDatabaseManager.sqlite (stage 2) | Written, no processing on 26.3 |
| Write EPUB payload for path traversal | Written, no processing on 26.3 |

### Recovery Mode (entered successfully)
| Attempt | Result |
|---------|--------|
| setenv activation-override true | Accepted, doesn't survive boot |
| setenv skip-setup true | Accepted, doesn't survive boot |
| saveenv + reboot | Reboots but env vars not effective |
| Read NONC and SNON nonces | Successful (nonces captured) |

### Other Services
| Service | Status |
|---------|--------|
| MCInstall (profile installer) | InvalidService (locked out) |
| afc2 (root filesystem) | InvalidService |
| webinspector | InvalidService |
| misagent | InvalidService |
| pcapd (packet capture) | Available |
| syslog_relay | Available |
| diagnostics_relay (reboot) | Available and working |
| crashreportcopymobile | Available |

## The Fundamental Barrier

**The activation nonce.** Every activation path on iOS 26.3 requires a
cryptographically valid activation nonce. This nonce is generated ONLY by
the FairPlay DRM engine inside mobileactivationd during a successful DRM
session. The DRM session refuses to establish when the device is iCloud-
locked because the FairPlay engine checks the lock status LOCALLY.

Without the nonce, ANY activation record we submit is rejected with
"Invalid activation nonce" (Error Code -1).

Apple also REMOVED the factory activation commands on 26.3, which was
the bypass used by Checkm8.info and iRemoveTools on iOS <= 26.1.

## What Would Be Needed

To bypass activation on iOS 26.3, one of the following would be required:

1. **A way to generate/obtain a valid activation nonce** without the DRM
   session. This would require either:
   - Breaking FairPlay's DRM crypto
   - Finding a bug in the nonce validation
   - Replaying a nonce from a different device (nonces are device-specific)

2. **A vulnerability in mobileactivationd** that allows bypassing the
   nonce check. This would require:
   - Reverse-engineering mobileactivationd from the iOS 26.3 firmware
   - Finding a code path that skips nonce validation
   - The firmware is encrypted (AEA) and we can't extract it without keys

3. **A kernel exploit** that allows patching mobileactivationd in memory
   to skip the nonce check. Would require:
   - An unpatched kernel vulnerability on iOS 26.3
   - None are publicly known as of April 2026

4. **The VU#346053 captive portal approach** which targets a DIFFERENT
   endpoint (humb.apple.com/humbug/baa) that may not require the nonce.
   This requires:
   - The device to connect to our WiFi during setup (needs user tap)
   - Our captive portal to intercept the activation flow
   - Cannot be automated over USB alone

## Captured Data (for future research)

Saved in captive_portal/server/requests/:
- real_handshake.plist -- Apple's actual drmHandshake response
- handshake_to_apple.plist -- Device's session blob sent to Apple
- Device nonces from recovery mode:
  - NONC: 778811943b98783af5152eca53e1953e5be528f69a73c645f19c1347d6d45885
  - SNON: 144dd9b8aa16736234922633ae9c48f4b53127cd
