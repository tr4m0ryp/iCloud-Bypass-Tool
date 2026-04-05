#!/usr/bin/env python3
"""
DNS configuration generator for captive portal.

Generates a dnsmasq.conf that redirects Apple activation and captive
portal domains to the local server.  Also configures DHCP so connected
devices use this server as their gateway and DNS resolver.

Usage:
    python3 dns_config.py [server_ip] [output_file]

Defaults:
    server_ip   = 192.168.1.1
    output_file  = stdout
"""

import os
import sys
from datetime import datetime

# ---------------------------------------------------------------------------
# Domains that must resolve to our server
# ---------------------------------------------------------------------------
REDIRECT_DOMAINS = [
    # Captive portal detection
    "captive.apple.com",
    "www.apple.com",
    "www.appleiphonecell.com",
    "www.itools.info",
    "www.ibook.info",
    "www.airport.us",
    "www.thinkdifferent.us",

    # Activation servers
    "albert.apple.com",
    "humb.apple.com",

    # Software / catalog / provisioning
    "gs.apple.com",
    "mesu.apple.com",
    "osrecovery.apple.com",
    "isu.apple.com",

    # Identity services (optional, for deeper interception)
    "identity.ess.apple.com",
    "setup.icloud.com",
]


def generate_dnsmasq_conf(
    server_ip="192.168.1.1",
    dhcp_start=None,
    dhcp_end=None,
    log_path=None,
):
    """
    Generate dnsmasq configuration text.

    Args:
        server_ip:  IP address of this server on the test network.
        dhcp_start: First address in DHCP pool (default: server_ip range .100).
        dhcp_end:   Last address in DHCP pool  (default: server_ip range .200).
        log_path:   Path for DNS query log (default: alongside this script).

    Returns:
        str: Complete dnsmasq.conf content.
    """
    # Derive DHCP range from server IP if not specified
    prefix = ".".join(server_ip.split(".")[:3])
    if dhcp_start is None:
        dhcp_start = f"{prefix}.100"
    if dhcp_end is None:
        dhcp_end = f"{prefix}.200"
    if log_path is None:
        log_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "dns.log"
        )

    lines = []
    lines.append(f"# dnsmasq configuration for captive portal")
    lines.append(f"# Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"# Server IP: {server_ip}")
    lines.append("")

    # -- DNS redirection --------------------------------------------------
    lines.append("# ---- Domain redirection ----")
    lines.append("# Redirect Apple activation + captive portal domains")
    for domain in REDIRECT_DOMAINS:
        lines.append(f"address=/{domain}/{server_ip}")
    lines.append("")

    # -- DHCP configuration -----------------------------------------------
    lines.append("# ---- DHCP ----")
    lines.append(f"dhcp-range={dhcp_start},{dhcp_end},12h")
    lines.append(f"dhcp-option=3,{server_ip}    # Default gateway")
    lines.append(f"dhcp-option=6,{server_ip}    # DNS server")
    lines.append("")

    # -- General settings -------------------------------------------------
    lines.append("# ---- General ----")
    lines.append("no-resolv")
    lines.append("no-hosts")
    lines.append(f"listen-address={server_ip},127.0.0.1")
    lines.append("bind-interfaces")
    lines.append("")

    # -- Logging ----------------------------------------------------------
    lines.append("# ---- Logging ----")
    lines.append("log-queries")
    lines.append(f"log-facility={log_path}")
    lines.append("")

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------
def main():
    server_ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.1"
    output_file = sys.argv[2] if len(sys.argv) > 2 else None

    conf = generate_dnsmasq_conf(server_ip=server_ip)

    if output_file:
        with open(output_file, "w") as f:
            f.write(conf)
        print(f"Written to {output_file}")
    else:
        print(conf)


if __name__ == "__main__":
    main()
