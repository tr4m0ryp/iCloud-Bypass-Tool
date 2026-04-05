# VulnHunt State

## Target
Install an unsigned iOS version (26.0 or earlier) on a device running iOS 26.2+.
This enables using existing tr4mpass bypass methods (Path A / Path B) which work
on iOS <= 26.1 but fail on 26.2+.

## Device
Not currently connected. Target: iPad (model TBD when connected).
Running: iOS 26.2+ (exact version TBD).

## Objective
Find a vulnerability or method that allows downgrading iOS to an unsigned
firmware version (26.0 or 26.1), bypassing Apple's signing server (TSS) checks.

## Attack Surface
- DFU mode firmware restore process
- Apple TSS (Tatsu Signing Server) at gs.apple.com/TSS/controller
- SHSH blob / APTicket validation
- iBoot firmware loading chain
- OTA update mechanism
- Supervised device management profiles
- SEP (Secure Enclave) forward-compatibility requirements
- Recovery mode restore flow

## Constraints
- No device connected currently -- research phase only
- A12+ devices have no bootrom exploit (checkm8 is A5-A11 only)
- Apple stopped signing iOS 26.0/26.1 -- TSS returns error for those versions
- SEP firmware must be compatible with the target iOS version
- Nonce-based replay protection on APTickets

## Current Cycle: 3
## Current Phase: ANALYZE -- EXPLOIT CHAIN IDENTIFIED
## Pivot: Downgrade is dead. Three captive portal attack chains identified.
## Next: Build test captive portal and verify with device.
