# VulnHunt Ideas Queue

## PIVOT: Direct bypass on iOS 26.2+ (downgrade path is dead)

### P1 (High Priority -- Viable Paths)
- ID-012: VU#346053 captive portal XML injection -- implement and test the
  activation endpoint vulnerability. No downgrade needed, no jailbreak needed.
  Builds on existing captive_portal/ infrastructure. (SEE 26.3-vulnerability.md)
- ID-013: MobileGestalt exploit on iOS 26.2 GA -- verify if the vulnerability
  patched in 26.2 Beta 2 is present in 26.2 GA release. If so, could enable
  code execution for activation bypass without downgrade.
- ID-014: DarkSword exploit chain adaptation -- 6-CVE chain targeting iOS 18.4-18.7.
  Research if any of the 6 vulnerabilities also affect iOS 26.x. Could provide
  kernel RCE leading to activation bypass.

### P2 (Medium Priority -- Research Needed)
- ID-015: iOS 26.2 specific CVE analysis -- diff iOS 26.1 vs 26.2 security
  notes to find what was patched. 1-day exploits in 26.1 that are fixed in 26.2
  might work on 26.2 if the fix was incomplete.
- ID-016: Setup Assistant escape -- instead of bypassing activation, can we
  escape Setup.app into a usable state on 26.2+? Research any UI-based escapes.
- ID-017: Activation endpoint response crafting -- instead of XML injection,
  can we craft a valid-looking activation response that the device accepts?
  Analyze the exact validation the device performs on activation responses.

### P3 (Low Priority -- Long Shots)
- ID-018: Monitor for iOS 26.x jailbreak -- if a jailbreak drops, all existing
  Path A/B methods plus futurerestore become viable again.
- ID-019: Hardware-level attack -- JTAG, chip-off, voltage glitching on A12+.
  Requires specialized equipment. Academic interest only.

## Completed/Dead (from Cycle 1)
- ~~ID-001: SHSH blob replay~~ -> DE-001
- ~~ID-002: TSS server bypass~~ -> DE-002
- ~~ID-003: Delayed OTA profile~~ -> DE-003
- ~~ID-004: SEP compatibility~~ -> DE-005
- ~~ID-005: iBoot nonce setter~~ -> DE-004
- ~~ID-006: Recovery mode TSS~~ -> DE-002
- ~~ID-007: OTA manifest manipulation~~ -> DE-003
- ~~ID-008: Bootrom exploits A12+~~ -> DE-004
- ~~ID-009: Apple Configurator downgrade~~ -> DE-003
- ~~ID-010: Frankenstein restore~~ -> DE-005
- ~~ID-011: futurerestore analysis~~ -> DE-006
