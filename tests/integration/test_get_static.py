"""
Integration tests: static file serving via GET.

Verifies that webserv correctly serves HTML, CSS, and binary (PNG) files
with appropriate status codes and Content-Type headers.
"""

import os
import sys
import shutil
import struct
import unittest
import zlib

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

# Directory that the single_server.conf fixture uses as document root
STATIC_ROOT = "/tmp/webserv_test_static"


def _minimal_png():
    """
    Build the smallest valid 1x1 white PNG entirely from stdlib.

    PNG spec: signature + IHDR + IDAT + IEND.
    """
    signature = b"\x89PNG\r\n\x1a\n"

    # IHDR: 1x1, 8-bit RGBA
    ihdr_data = struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)
    ihdr = _png_chunk(b"IHDR", ihdr_data)

    # IDAT: one row, filter byte 0, then R G B = 0xFF 0xFF 0xFF
    raw_row = b"\x00\xff\xff\xff"
    compressed = zlib.compress(raw_row)
    idat = _png_chunk(b"IDAT", compressed)

    # IEND
    iend = _png_chunk(b"IEND", b"")

    return signature + ihdr + idat + iend


def _png_chunk(chunk_type, data):
    """Create a single PNG chunk: length + type + data + CRC."""
    length = struct.pack(">I", len(data))
    crc = struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    return length + chunk_type + data + crc


class TestGetStatic(WebservTestCase):
    """GET requests for static HTML, CSS, and PNG files."""

    config_fixture = "single_server.conf"
    server_port = 8401

    def setUp(self):
        # Create document root and seed files BEFORE starting the server.
        if os.path.exists(STATIC_ROOT):
            shutil.rmtree(STATIC_ROOT)
        os.makedirs(STATIC_ROOT)

        with open(os.path.join(STATIC_ROOT, "index.html"), "w") as f:
            f.write("<html><body>Hello</body></html>\n")

        with open(os.path.join(STATIC_ROOT, "style.css"), "w") as f:
            f.write("body { margin: 0; }\n")

        with open(os.path.join(STATIC_ROOT, "image.png"), "wb") as f:
            f.write(_minimal_png())

        # Now start the server (base class setUp)
        super(TestGetStatic, self).setUp()

    def tearDown(self):
        # Kill the server first, then clean up temp files.
        try:
            super(TestGetStatic, self).tearDown()
        finally:
            if os.path.exists(STATIC_ROOT):
                shutil.rmtree(STATIC_ROOT)

    # ------------------------------------------------------------------ tests

    def test_serves_html(self):
        status, headers, body = self.http_get("/index.html")
        self.assertEqual(status, 200)
        self.assertIn("text/html", headers.get("content-type", ""))
        self.assertIn(b"Hello", body)

    def test_serves_css(self):
        status, headers, body = self.http_get("/style.css")
        self.assertEqual(status, 200)
        self.assertIn("text/css", headers.get("content-type", ""))
        self.assertIn(b"margin", body)

    def test_serves_png(self):
        status, headers, body = self.http_get("/image.png")
        self.assertEqual(status, 200)
        self.assertIn("image/png", headers.get("content-type", ""))
        # PNG files always start with the 8-byte signature
        self.assertTrue(body.startswith(b"\x89PNG"))


if __name__ == "__main__":
    unittest.main()
