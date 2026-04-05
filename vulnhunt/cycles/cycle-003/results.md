# Cycle 3 Results (Partial -- binary analysis still running)

## BREAKTHROUGH: WebSheet Captive Portal Attack Chain

### Discovery
WebSheet (the captive portal browser) has TWO privileged entitlements that
can be triggered DURING the activation lock screen:

1. `com.apple.managedconfiguration.profiled-access` -- silent profile install
2. `com.apple.springboard.opensensitiveurl` -- launch apps via URL scheme

### Why This Matters
WebSheet is automatically triggered when iOS detects a captive portal.
This happens during Setup Assistant BEFORE activation. If we control the
WiFi network, we control what WebSheet loads. No user interaction needed.

### The HTTP Probe
iOS probes `http://captive.apple.com/hotspot-detect.html` over plain HTTP.
Expected response: `<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>`
Any other response triggers WebSheet to display our content.

### Attack Chain (No WebKit Exploit Needed)
1. Set up rogue WiFi AP with DNS spoofing
2. Activation-locked device connects during Setup Assistant
3. DNS redirects captive.apple.com to our server
4. We serve a non-"Success" page -> WebSheet auto-opens
5. Our page triggers configuration profile installation
   (WebSheet has profiled-access entitlement for this)
6. Profile installs: custom Root CA + MDM enrollment payload
7. With trusted Root CA installed, we can MITM HTTPS connections
8. MITM albert.apple.com activation endpoint
9. Serve crafted activation response -> device accepts it
10. Activation lock bypassed

### Why This Might Work on iOS 26.3+
- Uses LEGITIMATE WebSheet entitlements, not a patched vulnerability
- The profiled-access entitlement exists so enterprise WiFi captive
  portals can install WiFi configuration profiles
- Apple cannot remove this entitlement without breaking ALL enterprise
  captive portals worldwide
- The HTTP probe (not HTTPS) has existed since iOS 7 and persists through
  iOS 26 for backward compatibility

### Open Questions
- Does profiled-access work during Setup Assistant specifically?
  (It works during normal operation -- need to verify setup flow)
- Does the profile installation require ANY user confirmation on 26.3?
  (Historical: iOS 9.2.1 added isolated cookie store but not profile gates)
- Does iOS 26.3 still use the HTTP probe? (iOS 14+ added RFC 8910 DHCP
  option as alternative, but HTTP probe may still be active for compat)
- Can we install a Root CA profile that is trusted for TLS?
  (iOS 10.3+ requires user to manually trust installed CAs in Settings)

### Historical Precedent
Marco Grassi (2016) demonstrated this EXACT chain:
WebSheet WebKit exploit -> profiled-access for Root CA -> MITM HTTPS
-> second exploit in unsandboxed app -> code execution outside sandbox.

Our variation skips the WebKit exploit for the profile install step,
but the MITM portion is the same concept.

### Risk Assessment
The CA trust requirement (iOS 10.3+ needs manual trust in Settings) is
the main uncertainty. During Setup Assistant, the user may not have access
to Settings > Certificate Trust Settings. However:
- MDM-enrolled profiles with Root CA may auto-trust
- The VU#346053 injection doesn't need HTTPS MITM -- it targets the
  activation endpoint which may have its own trust handling

## Status
Binary analysis agents still running. Will combine WebSheet findings
with binary RE results in the next update.
