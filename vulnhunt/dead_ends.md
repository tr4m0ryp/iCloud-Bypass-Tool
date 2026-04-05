# VulnHunt Dead Ends

## DE-001: SHSH Blob Downgrade (ID-001)
**Ruled out:** Cryptex1 (iOS 16+) breaks all A12+ downgrades. SHSH blobs for
iOS 26.0 cannot be obtained retroactively. Even with blobs, futurerestore
cannot handle Cryptex1 nonce seeds. Nonce cannot be set without jailbreak.
**Revisit condition:** If Cryptex1 nonce persistence is discovered, or if a
jailbreak for iOS 26.2+ emerges.

## DE-002: TSS Server Bypass/MITM (ID-002)
**Ruled out:** TSS uses cryptographic signature verification. No known
vulnerability in Apple's signing infrastructure. APTickets are signed with
Apple's private key -- cannot be forged. Historical cache attacks patched
since iOS 5+.
**Revisit condition:** If a TSS infrastructure vulnerability is disclosed.

## DE-003: OTA/MDM Downgrade (ID-003, ID-007, ID-009)
**Ruled out:** All OTA paths are cryptographically signed (RSA-2048) and
enforce strict forward-only version checking. MDM explicitly cannot
downgrade. Apple Configurator only installs signed IPSW. Delayed OTA
profiles only defer, never downgrade. Supervision requires device wipe.
**Revisit condition:** None -- architectural limitation.

## DE-004: A12+ Bootrom Exploit (ID-008)
**Ruled out:** No bootrom vulnerability found for A12+ in 7+ years since
checkm8. A12 introduced PAC randomization in SecureROM. iBoot requires
bootrom access. SEP has no A12+ exploit. DFU enforces signatures.
PongoOS requires checkm8. Kernel exploits don't reach bootchain.
**Revisit condition:** If a new A12+ bootrom vulnerability is disclosed.

## DE-005: Frankenstein Restore (ID-010)
**Ruled out:** Requires either bootrom exploit or jailbreak to set nonce.
Neither exists for A12+ on iOS 26. SEP compatibility between 26.0 and
26.2 is unknown and likely incompatible. Cryptex1 adds another layer.
**Revisit condition:** Same as DE-001.

## DE-006: futurerestore (ID-011)
**Ruled out:** Broken on A12+ for iOS 16+ due to Cryptex1. No alternative
tools work on A12+ either (sunst0rm, Inferius, Odysseus all A11-only).
**Revisit condition:** If Cryptex1 research yields a breakthrough.

## CONCLUSION
The firmware downgrade approach is a dead end for A12+ on iOS 26.2+.
Pivot to direct bypass on current iOS version instead of downgrading.
