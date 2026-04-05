#!/usr/bin/env python3
"""
Captive portal HTTP server for iOS activation research.

Simulates Apple's captive portal detection and activation endpoints:
  - captive.apple.com/hotspot-detect.html
  - albert.apple.com/deviceservices/*
  - humb.apple.com/humbug/baa

All requests are logged for analysis.
"""

import os
import sys
import json
import time
import logging
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, unquote_plus

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
PAYLOADS_DIR = os.path.join(PROJECT_DIR, "payloads")
ACCESS_LOG = os.path.join(SCRIPT_DIR, "access.log")
REQUESTS_DIR = os.path.join(SCRIPT_DIR, "requests")

os.makedirs(REQUESTS_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
file_handler = logging.FileHandler(ACCESS_LOG)
file_handler.setFormatter(logging.Formatter(
    "%(asctime)s | %(message)s", datefmt="%Y-%m-%d %H:%M:%S"
))
stream_handler = logging.StreamHandler(sys.stdout)
stream_handler.setFormatter(logging.Formatter(
    "\033[36m%(asctime)s\033[0m | %(message)s", datefmt="%H:%M:%S"
))

logger = logging.getLogger("portal")
logger.setLevel(logging.DEBUG)
logger.addHandler(file_handler)
logger.addHandler(stream_handler)

# ---------------------------------------------------------------------------
# Lazy import of response builders
# ---------------------------------------------------------------------------
sys.path.insert(0, SCRIPT_DIR)
from activation_responses import (
    build_handshake_response,
    build_activation_record,
    build_baa_response,
)

# ---------------------------------------------------------------------------
# HTML templates
# ---------------------------------------------------------------------------
STATUS_PAGE = """\
<!DOCTYPE html>
<html><head><title>Portal Status</title>
<style>
  body{font-family:Helvetica,Arial,sans-serif;background:#111;color:#eee;
       display:flex;align-items:center;justify-content:center;height:100vh;margin:0}
  .box{text-align:center;border:1px solid #333;padding:2em 3em;border-radius:8px}
  h1{margin:0 0 .5em}
  p{color:#888}
</style></head>
<body><div class="box">
  <h1>Captive Portal Server</h1>
  <p>Running -- all endpoints operational.</p>
  <p style="font-size:.8em;color:#555">Started: %(started)s</p>
</div></body></html>
"""

CAPTIVE_PAGE = """\
<!DOCTYPE html>
<html><head><title>Network Authentication</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
  body{font-family:-apple-system,Helvetica,Arial,sans-serif;background:#f5f5f7;
       color:#1d1d1f;margin:0;padding:2em;text-align:center}
  .card{background:#fff;border-radius:12px;padding:2em;max-width:400px;
        margin:3em auto;box-shadow:0 2px 12px rgba(0,0,0,.08)}
  h1{font-size:1.3em;margin-bottom:.5em}
  p{color:#666;line-height:1.5}
  a.btn{display:inline-block;margin-top:1.5em;padding:.8em 2em;
        background:#0071e3;color:#fff;border-radius:8px;text-decoration:none;
        font-weight:600}
  a.btn:hover{background:#005bb5}
  .note{font-size:.75em;color:#999;margin-top:1.5em}
</style></head>
<body>
<div class="card">
  <h1>Network Configuration Required</h1>
  <p>This network requires authentication. Install the network profile
     to continue.</p>
  <a class="btn" href="/profile.mobileconfig">Install Profile</a>
  <p class="note">The profile configures your device for secure access
     to this network.</p>
</div>
</body></html>
"""

SERVER_START_TIME = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# ---------------------------------------------------------------------------
# Request body logger
# ---------------------------------------------------------------------------
_request_counter = 0


def _save_request_body(method, path, body):
    """Persist request body to individual file for later analysis."""
    global _request_counter
    _request_counter += 1
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    safe_path = path.strip("/").replace("/", "_") or "root"
    fname = f"{ts}_{_request_counter}_{method}_{safe_path}.bin"
    fpath = os.path.join(REQUESTS_DIR, fname)
    with open(fpath, "wb") as f:
        f.write(body)
    return fpath


# ---------------------------------------------------------------------------
# Handler
# ---------------------------------------------------------------------------
class PortalHandler(BaseHTTPRequestHandler):
    """Routes requests to the appropriate handler."""

    # Silence default stderr logging (we handle it ourselves)
    def log_message(self, fmt, *args):
        logger.info("%s %s %s", self.address_string(), self.command, self.path)

    # -- Helpers ----------------------------------------------------------

    def _send(self, code, content_type, body):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(length) if length else b""

    # -- GET routes -------------------------------------------------------

    def do_GET(self):
        path = self.path.split("?")[0]

        if path == "/":
            self._handle_status()
        elif path == "/hotspot-detect.html":
            self._handle_hotspot()
        elif path == "/captive-portal":
            self._handle_captive()
        elif path == "/profile.mobileconfig":
            self._handle_profile()
        else:
            self._send(404, "text/plain", "Not Found")

    def _handle_status(self):
        html = STATUS_PAGE % {"started": SERVER_START_TIME}
        self._send(200, "text/html", html)

    def _handle_hotspot(self):
        # Apple expects the literal string "<HTML><HEAD><TITLE>Success</TITLE>"
        # to dismiss the WebSheet.  Returning anything else keeps it open.
        logger.info("  >> Captive detection hit -- returning portal page")
        self._send(200, "text/html", CAPTIVE_PAGE)

    def _handle_captive(self):
        self._send(200, "text/html", CAPTIVE_PAGE)

    def _handle_profile(self):
        profile_path = os.path.join(PAYLOADS_DIR, "network.mobileconfig")
        if not os.path.isfile(profile_path):
            logger.warning("  >> Profile not found at %s", profile_path)
            self._send(404, "text/plain",
                        "Profile not built yet. Run payloads/build_profile.py first.")
            return
        with open(profile_path, "rb") as f:
            data = f.read()
        logger.info("  >> Serving profile (%d bytes)", len(data))
        self._send(200, "application/x-apple-aspen-config", data)

    # -- POST routes ------------------------------------------------------

    def do_POST(self):
        path = self.path.split("?")[0]
        body = self._read_body()
        saved = _save_request_body("POST", path, body)
        logger.info("  >> POST body saved to %s (%d bytes)", saved, len(body))

        if path == "/deviceservices/drmHandshake":
            self._handle_handshake(body)
        elif path == "/deviceservices/deviceActivation":
            self._handle_activation(body)
        elif path == "/humbug/baa":
            self._handle_baa(body)
        else:
            self._send(404, "text/plain", "Not Found")

    def _handle_handshake(self, body):
        logger.info("  >> drmHandshake request received")
        resp = build_handshake_response()
        self._send(200, "application/x-apple-plist", resp)

    def _handle_activation(self, body):
        logger.info("  >> deviceActivation request received")
        # Parse URL-encoded body to extract activation-info
        try:
            params = parse_qs(body.decode("utf-8", errors="replace"))
            device_info = params.get("activation-info", [b""])[0]
        except Exception:
            device_info = body
        resp = build_activation_record(device_info)
        self._send(200, "application/x-apple-plist", resp)

    def _handle_baa(self, body):
        logger.info("  >> humbug/baa request received (VU#346053 target)")
        resp = build_baa_response()
        self._send(200, "application/x-apple-plist", resp)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    host = "0.0.0.0"
    port = int(os.environ.get("PORTAL_PORT", 80))

    print("\033[1;32m" + "=" * 56)
    print("  Captive Portal Server")
    print("=" * 56 + "\033[0m")
    print(f"  Listening on {host}:{port}")
    print(f"  Access log:  {ACCESS_LOG}")
    print(f"  Request log: {REQUESTS_DIR}/")
    print()

    try:
        server = HTTPServer((host, port), PortalHandler)
        server.serve_forever()
    except PermissionError:
        print("\033[1;31mError: Port 80 requires root. Run with sudo.\033[0m")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\033[33mShutting down.\033[0m")
        server.server_close()


if __name__ == "__main__":
    main()
