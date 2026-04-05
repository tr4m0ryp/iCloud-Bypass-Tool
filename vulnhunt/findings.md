# VulnHunt Findings

## F-001: iOS Downgrade is Dead on A12+ (Cycle 1)
**Evidence:** All 5 research vectors independently confirmed firmware downgrade
on A12+ is not feasible. Cryptex1 is the primary blocker.
**Implication:** Pivot to direct bypass on current iOS version.

## F-002: MobileGestalt / bookassetd Sandbox Escape (Cycle 1+2)
**Evidence:** Researcher Hana Kim published full PoC exploiting itunesstored
and bookassetd daemons. Affects iOS 18.6 through 26.2 Beta 1. Patched in
26.2 Beta 2. Integrated into iRemoval Pro. 26.2 GA status UNCERTAIN --
may still be vulnerable depending on release timing vs beta patch merge.
**Source:** https://hanakim3945.github.io/posts/download28_sbx_escape/
**Implication:** If 26.2 GA is vulnerable, this provides sandbox escape
without needing WebKit RCE.

## F-003: VU#346053 XML Injection (Cycle 1)
**Evidence:** Unpatched activation endpoint vulnerability at
humb.apple.com/humbug/baa. Accepts unsigned XML payloads.
**Source:** https://github.com/JGoyd/iOS-Activation-Flaw
**Implication:** Direct activation bypass via captive portal. Works on all
iOS versions. No jailbreak or exploit needed. PRIMARY VECTOR.

## F-004: DarkSword 6-CVE Exploit Chain (Cycle 2)
**Evidence:** Google TAG published analysis of DarkSword exploit kit:
- CVE-2025-31277: JavaScriptCore type confusion (patched 26.1)
- CVE-2025-43529: WebKit UAF (patched 26.2)
- CVE-2025-14174: WebKit ANGLE memory corruption (patched 26.2)
- CVE-2026-20700: dyld PAC bypass (patched 26.3)
- CVE-2025-43510: XNU kernel COW bug (patched 26.1)
- CVE-2025-43520: XNU kernel VFS race (patched 26.1)
Full chain: Safari RCE -> sandbox escape -> kernel write -> device compromise.
iOS 26.0 vulnerable to ALL 6. iOS 26.1 vulnerable to 3. iOS 26.2 to 1 (PAC).
**Source:** Google Cloud Blog, The Hacker News
**Implication:** WebKit RCE during captive portal setup on 26.0-26.1.

## F-005: WebSheet Attack Vector During Setup (Cycle 2) -- BREAKTHROUGH
**Evidence:** During activation setup, iOS connects to WiFi and probes for
captive portal via WebSheet daemon. WebSheet runs WebKit in a LOWER sandbox
than MobileSafari (no split-process model). If we control the WiFi AP, we
control what WebSheet loads.
**Attack chain:**
1. Device connects to our rogue WiFi during Setup Assistant
2. iOS probes captive.apple.com/hotspot-detect.html
3. We respond with captive portal redirect
4. WebSheet loads our page (with WebKit exploit payload)
5. WebKit RCE achieved in WebSheet process
6. Sandbox escape (MobileGestalt or DarkSword chain)
7. Modify activation state or inject activation records
**Implication:** No downgrade, no bootrom exploit, no jailbreak needed.
The captive portal is our entry point during setup.

## F-006: CVE-2026-20700 dyld PAC Bypass (Cycle 2)
**Evidence:** Present on iOS 26.0-26.2, patched in 26.3. Bypasses Pointer
Authentication Codes in dyld. Part of DarkSword chain but independently
useful for escalating from WebKit RCE to arbitrary code execution.
**Implication:** On iOS 26.2, this is the ONLY remaining DarkSword CVE.
Combined with a WebKit entry point, it enables code execution.

## F-007: CVE-2026-28895 Stolen Device Protection Bypass (Cycle 2)
**Evidence:** Affects iOS 26.0-26.3, patched in 26.4. Allows bypassing
biometric gating with just the device passcode.
**Implication:** If device passcode is known, this bypasses Face ID
requirements for protected operations.

## F-008: iOS Version Vulnerability Matrix (Cycle 2)
| iOS    | WebKit RCE | Sandbox Escape | Kernel | PAC Bypass | Activation |
|--------|-----------|---------------|--------|-----------|------------|
| 26.0   | YES (3 CVEs) | YES (MobileGestalt) | YES (2 CVEs) | YES | VU#346053 |
| 26.1   | YES (2 CVEs) | YES (MobileGestalt) | NO | YES | VU#346053 |
| 26.2   | NO (patched) | MAYBE (GA?) | NO | YES | VU#346053 |
| 26.3   | NO | NO | NO | NO | VU#346053 |
| 26.3.1+| NO | NO | NO | NO | VU#346053 |

**Key insight:** VU#346053 works on ALL versions. For 26.0-26.1, we also
have full exploit chains via WebSheet. For 26.2+, VU#346053 is the
primary (and possibly only) vector.
