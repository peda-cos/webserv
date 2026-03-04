"""
Stress test: rapid connect / disconnect cycles.

Verifies that the server survives 500 TCP connections that are opened
and immediately closed, and still serves valid responses afterward.
"""

import os
import sys
import shutil
import socket
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

STATIC_ROOT = "/tmp/webserv_test_static"
NUM_CYCLES = 500
SOCKET_TIMEOUT = 5.0


class TestRapidConnectDisconnect(WebservTestCase):
    """Open and immediately close 500 TCP connections, then verify the server."""

    config_fixture = "single_server.conf"
    server_port = 8401

    def setUp(self):
        # Create document root BEFORE starting the server.
        if os.path.exists(STATIC_ROOT):
            shutil.rmtree(STATIC_ROOT)
        os.makedirs(STATIC_ROOT)

        with open(os.path.join(STATIC_ROOT, "index.html"), "w") as f:
            f.write("<html><body>OK</body></html>\n")

        super(TestRapidConnectDisconnect, self).setUp()

    def tearDown(self):
        super(TestRapidConnectDisconnect, self).tearDown()
        try:
            if os.path.exists(STATIC_ROOT):
                shutil.rmtree(STATIC_ROOT)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_500_rapid_connect_disconnect(self):
        """Open+close 500 sockets, then confirm the server still responds."""
        connection_errors = 0

        for i in range(NUM_CYCLES):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(SOCKET_TIMEOUT)
                sock.connect((self.server_host, self.server_port))
                sock.close()
            except (socket.error, OSError):
                connection_errors += 1
                # Some failures are acceptable (e.g. connection refused
                # under heavy load) as long as the server recovers.
                continue

        # Allow a small fraction of connect failures under load,
        # but the majority must succeed.
        max_acceptable_errors = NUM_CYCLES // 10  # 10 %
        self.assertLessEqual(
            connection_errors, max_acceptable_errors,
            "%d of %d connect attempts failed (limit %d)" % (
                connection_errors, NUM_CYCLES, max_acceptable_errors
            ),
        )

        # Give the server a moment to process the closed connections
        time.sleep(0.2)

        # A real request must still succeed
        status, headers, body = self.http_get("/")
        self.assertEqual(
            status, 200,
            "Expected 200 after rapid connect/disconnect, got %d" % status,
        )

        # Server process must still be running
        self.assertTrue(
            self.server_is_alive(),
            "Server process died during rapid connect/disconnect test",
        )


if __name__ == "__main__":
    unittest.main()
