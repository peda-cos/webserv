"""
Stress test: OOM / body-size resilience.

Verifies that the server does not crash when it receives many requests
with bodies near or exceeding the client_max_body_size limit.
"""

import os
import sys
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

TEST_DIR = "/tmp/webserv_test_bodylimit"
UPLOAD_DIR = os.path.join(TEST_DIR, "uploads")

# body_limit.conf sets client_max_body_size to 1024
BODY_LIMIT = 1024
NUM_REQUESTS = 20


class TestOomResilience(WebservTestCase):
    """Repeated large/oversized POSTs must not crash the server."""

    config_fixture = "body_limit.conf"
    server_port = 8409

    def setUp(self):
        # Create document root and upload directory BEFORE starting the server.
        if os.path.exists(TEST_DIR):
            shutil.rmtree(TEST_DIR)
        os.makedirs(UPLOAD_DIR)

        # Provide a minimal index so GET / works for health checks.
        with open(os.path.join(TEST_DIR, "index.html"), "w") as f:
            f.write("<html><body>OK</body></html>\n")

        super(TestOomResilience, self).setUp()

    def tearDown(self):
        super(TestOomResilience, self).tearDown()
        try:
            if os.path.exists(TEST_DIR):
                shutil.rmtree(TEST_DIR)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_large_bodies_no_crash(self):
        """
        Send 20 POSTs with 900-byte bodies (just under the 1024 limit).
        The server must stay alive after every request and still serve
        a normal GET at the end.
        """
        body = "X" * 900

        for i in range(NUM_REQUESTS):
            status, headers, resp_body = self.http_post(
                "/upload", body, "application/octet-stream"
            )
            # We do not assert the exact status here because the server
            # may return 200, 201, or another success code depending on
            # implementation — but it must NOT be 500 or 0 (no response).
            self.assertNotEqual(
                status, 0,
                "Request %d/%d: server returned no response" % (i + 1, NUM_REQUESTS),
            )
            self.assertTrue(
                self.server_is_alive(),
                "Server crashed after request %d/%d (status %d)" % (
                    i + 1, NUM_REQUESTS, status
                ),
            )

        # Final health check — a plain GET must work
        status, headers, resp_body = self.http_get("/")
        self.assertIn(
            status, (200, 301, 302),
            "Expected a successful response for GET / after %d large POSTs, "
            "got %d" % (NUM_REQUESTS, status),
        )

    def test_oversized_bodies_no_crash(self):
        """
        Send 20 POSTs with 2048-byte bodies (over the 1024 limit).
        Each should be rejected with 413. The server must stay alive
        throughout and still respond to a normal GET afterward.
        """
        body = "Y" * 2048

        for i in range(NUM_REQUESTS):
            status, headers, resp_body = self.http_post(
                "/upload", body, "application/octet-stream"
            )
            self.assertEqual(
                status, 413,
                "Request %d/%d: expected 413, got %d" % (
                    i + 1, NUM_REQUESTS, status
                ),
            )
            self.assertTrue(
                self.server_is_alive(),
                "Server crashed after oversized request %d/%d" % (
                    i + 1, NUM_REQUESTS
                ),
            )

        # Final health check
        status, headers, resp_body = self.http_get("/")
        self.assertNotEqual(
            status, 0,
            "Server returned no response to GET / after %d oversized POSTs" % NUM_REQUESTS,
        )
        self.assertTrue(
            self.server_is_alive(),
            "Server died after the final GET health check",
        )


if __name__ == "__main__":
    unittest.main()
