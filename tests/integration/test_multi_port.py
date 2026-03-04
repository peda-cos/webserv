"""
Integration test: multiple server blocks on different ports.

Verifies that two server blocks listening on distinct ports each serve
their own document root with distinct content.
"""

import sys
import os
import time
import socket
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase, SERVER_START_TIMEOUT, SERVER_START_POLL_INTERVAL

TEST_DIR_A = "/tmp/webserv_test_port_a"
TEST_DIR_B = "/tmp/webserv_test_port_b"
PORT_A = 8402
PORT_B = 8403


class TestMultiPort(WebservTestCase):
    config_fixture = "multi_port.conf"
    server_port = PORT_A  # primary port used by base-class health check

    def setUp(self):
        # Create both document roots with distinct content BEFORE
        # starting the server.
        for d in (TEST_DIR_A, TEST_DIR_B):
            if os.path.exists(d):
                shutil.rmtree(d)
            os.makedirs(d)

        with open(os.path.join(TEST_DIR_A, "index.html"), "w") as f:
            f.write("PORT_A_CONTENT\n")

        with open(os.path.join(TEST_DIR_B, "index.html"), "w") as f:
            f.write("PORT_B_CONTENT\n")

        super(TestMultiPort, self).setUp()

    def tearDown(self):
        super(TestMultiPort, self).tearDown()
        for d in (TEST_DIR_A, TEST_DIR_B):
            try:
                if os.path.exists(d):
                    shutil.rmtree(d)
            except OSError:
                pass

    def _wait_for_server(self):
        """
        Override: wait for BOTH ports to accept connections so that
        tests against PORT_B don't race.
        """
        deadline = time.time() + SERVER_START_TIMEOUT
        ports_ready = set()

        while time.time() < deadline:
            if self._server_proc.poll() is not None:
                return False
            for port in (PORT_A, PORT_B):
                if port in ports_ready:
                    continue
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(1.0)
                    sock.connect((self.server_host, port))
                    sock.close()
                    ports_ready.add(port)
                except (socket.error, OSError):
                    pass
            if len(ports_ready) == 2:
                return True
            time.sleep(SERVER_START_POLL_INTERVAL)
        return False

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_port_a_serves_distinct_content(self):
        """GET / on PORT_A should return 200 with PORT_A_CONTENT."""
        status, headers, body = self.http_get("/", port=PORT_A)
        self.assertEqual(status, 200)

        body_text = body.decode("utf-8", errors="replace")
        self.assertIn("PORT_A_CONTENT", body_text)

    def test_port_b_serves_distinct_content(self):
        """GET / on PORT_B should return 200 with PORT_B_CONTENT."""
        status, headers, body = self.http_get("/", port=PORT_B)
        self.assertEqual(status, 200)

        body_text = body.decode("utf-8", errors="replace")
        self.assertIn("PORT_B_CONTENT", body_text)


if __name__ == "__main__":
    unittest.main()
