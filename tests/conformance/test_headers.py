"""
Conformance tests: HTTP response headers.

Verifies that webserv emits correct and well-formed HTTP headers
on every response: Content-Length accuracy, Content-Type presence,
no unsupported Transfer-Encoding, and a valid Connection header.

Uses the existing single_server.conf fixture on port 8401.
"""

import os
import sys
import shutil
import unittest

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

# The single_server.conf fixture serves from this directory.
STATIC_ROOT = "/tmp/webserv_test_static"


class TestHeaders(WebservTestCase):
    """Validate HTTP response headers against conformance requirements."""

    config_fixture = "single_server.conf"
    server_host = "127.0.0.1"
    server_port = 8401

    def setUp(self):
        # Create the document root and seed it with test files BEFORE
        # starting the server.
        if os.path.exists(STATIC_ROOT):
            shutil.rmtree(STATIC_ROOT)
        os.makedirs(STATIC_ROOT)

        with open(os.path.join(STATIC_ROOT, "index.html"), "w") as f:
            f.write("<html><body>header test</body></html>\n")

        with open(os.path.join(STATIC_ROOT, "data.txt"), "w") as f:
            f.write("Some plain text content for testing.\n")

        with open(os.path.join(STATIC_ROOT, "style.css"), "w") as f:
            f.write("body { color: black; }\n")

        super(TestHeaders, self).setUp()

    def tearDown(self):
        try:
            super(TestHeaders, self).tearDown()
        finally:
            if os.path.exists(STATIC_ROOT):
                shutil.rmtree(STATIC_ROOT)

    # ----------------------------------------------------------------
    # Tests
    # ----------------------------------------------------------------

    def test_content_length_matches_body(self):
        """Content-Length header must equal the actual body size."""
        status, headers, body = self.http_get("/index.html")
        self.assertEqual(status, 200, "Expected 200, got %d" % status)
        self.assertIn(
            "content-length", headers,
            "Response is missing Content-Length header",
        )
        declared = int(headers["content-length"])
        actual = len(body)
        self.assertEqual(
            declared, actual,
            "Content-Length %d does not match body length %d"
            % (declared, actual),
        )

    def test_content_length_matches_body_txt(self):
        """Content-Length accuracy check on a plain-text file."""
        status, headers, body = self.http_get("/data.txt")
        self.assertEqual(status, 200, "Expected 200, got %d" % status)
        self.assertIn("content-length", headers)
        declared = int(headers["content-length"])
        self.assertEqual(
            declared, len(body),
            "Content-Length %d != body length %d" % (declared, len(body)),
        )

    def test_content_type_on_200(self):
        """A successful GET must include a non-empty Content-Type header."""
        for path in ("/index.html", "/data.txt", "/style.css"):
            status, headers, body = self.http_get(path)
            self.assertEqual(
                status, 200,
                "Expected 200 for %s, got %d" % (path, status),
            )
            self.assertIn(
                "content-type", headers,
                "Missing Content-Type for %s" % path,
            )
            self.assertTrue(
                len(headers["content-type"]) > 0,
                "Empty Content-Type for %s" % path,
            )

    def test_content_type_values(self):
        """Content-Type should match the file extension's MIME type."""
        expectations = {
            "/index.html": "text/html",
            "/data.txt": "text/plain",
            "/style.css": "text/css",
        }
        for path, expected_mime in expectations.items():
            status, headers, body = self.http_get(path)
            self.assertEqual(status, 200)
            ct = headers.get("content-type", "")
            self.assertIn(
                expected_mime, ct,
                "Expected %s in Content-Type for %s, got '%s'"
                % (expected_mime, path, ct),
            )

    def test_no_chunked_transfer_encoding(self):
        """
        webserv is not required to support chunked transfer-encoding
        on responses.  Verify it does not appear.
        """
        for path in ("/index.html", "/data.txt", "/style.css"):
            status, headers, body = self.http_get(path)
            te = headers.get("transfer-encoding", "")
            self.assertNotIn(
                "chunked", te.lower(),
                "Unexpected chunked Transfer-Encoding on %s" % path,
            )

    def test_connection_header_valid(self):
        """
        If the server includes a Connection header, its value must be
        either 'close' or 'keep-alive' (case-insensitive).
        """
        status, headers, body = self.http_get("/index.html")
        self.assertEqual(status, 200)
        if "connection" in headers:
            value = headers["connection"].lower()
            self.assertIn(
                value, ("close", "keep-alive"),
                "Connection header has unexpected value: '%s'" % value,
            )

    def test_content_length_on_error(self):
        """Even error responses should have a Content-Length that matches."""
        status, headers, body = self.http_get("/does_not_exist_at_all")
        self.assertEqual(status, 404, "Expected 404, got %d" % status)
        if "content-length" in headers:
            declared = int(headers["content-length"])
            self.assertEqual(
                declared, len(body),
                "Content-Length %d != body length %d on 404"
                % (declared, len(body)),
            )


if __name__ == "__main__":
    unittest.main()
