"""
Stress test: concurrent connections.

Verifies that the server can handle 100 simultaneous GET requests
without dropping connections, timing out, or crashing.
"""

import os
import sys
import shutil
import socket
import threading
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

STATIC_ROOT = "/tmp/webserv_test_static"
NUM_THREADS = 100
SOCKET_TIMEOUT = 5.0


class TestConcurrentConnections(WebservTestCase):
    """100 threads each send a GET / and all must get 200 back."""

    config_fixture = "single_server.conf"
    server_port = 8401

    def setUp(self):
        # Create document root BEFORE starting the server.
        if os.path.exists(STATIC_ROOT):
            shutil.rmtree(STATIC_ROOT)
        os.makedirs(STATIC_ROOT)

        with open(os.path.join(STATIC_ROOT, "index.html"), "w") as f:
            f.write("<html><body>OK</body></html>\n")

        super(TestConcurrentConnections, self).setUp()

    def tearDown(self):
        super(TestConcurrentConnections, self).tearDown()
        try:
            if os.path.exists(STATIC_ROOT):
                shutil.rmtree(STATIC_ROOT)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _send_get(self, results, index):
        """
        Send a single GET / over a raw socket and store the result.

        Each entry in `results` is a dict with keys:
            status  - HTTP status code (int) or 0 on failure
            elapsed - wall-clock seconds the request took
            error   - None on success, exception string on failure
        """
        start = time.time()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(SOCKET_TIMEOUT)
            sock.connect((self.server_host, self.server_port))

            request = (
                "GET / HTTP/1.1\r\n"
                "Host: " + self.server_host + ":" + str(self.server_port) + "\r\n"
                "Connection: close\r\n"
                "\r\n"
            )
            sock.sendall(request.encode("utf-8"))

            response = b""
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    break

            sock.close()
            elapsed = time.time() - start

            # Parse status code from first line
            status = 0
            if response:
                first_line = response.split(b"\r\n", 1)[0].decode("utf-8", errors="replace")
                parts = first_line.split(" ", 2)
                if len(parts) >= 2:
                    try:
                        status = int(parts[1])
                    except ValueError:
                        pass

            results[index] = {"status": status, "elapsed": elapsed, "error": None}
        except Exception as exc:
            results[index] = {
                "status": 0,
                "elapsed": time.time() - start,
                "error": str(exc),
            }

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_100_concurrent_connections(self):
        """Launch 100 threads, each sending GET /; all must return 200."""
        results = [None] * NUM_THREADS
        threads = []

        for i in range(NUM_THREADS):
            t = threading.Thread(target=self._send_get, args=(results, i))
            threads.append(t)

        # Start all threads as close together as possible
        for t in threads:
            t.start()

        # Wait for every thread to finish
        for t in threads:
            t.join(timeout=SOCKET_TIMEOUT + 2.0)

        # ------ assertions ------

        # Every slot must have been filled
        for i, r in enumerate(results):
            self.assertIsNotNone(
                r, "Thread %d did not produce a result (likely hung)" % i
            )

        # Collect failures for a useful error message
        errors = []
        for i, r in enumerate(results):
            if r["error"] is not None:
                errors.append("Thread %d: %s" % (i, r["error"]))
            elif r["status"] != 200:
                errors.append(
                    "Thread %d: expected 200, got %d" % (i, r["status"])
                )

        self.assertEqual(
            len(errors), 0,
            "%d of %d requests failed:\n  %s" % (
                len(errors), NUM_THREADS, "\n  ".join(errors)
            ),
        )

        # No single request should have taken longer than the socket timeout
        for i, r in enumerate(results):
            self.assertLessEqual(
                r["elapsed"], SOCKET_TIMEOUT,
                "Thread %d took %.2fs (limit %.1fs)" % (
                    i, r["elapsed"], SOCKET_TIMEOUT
                ),
            )

        # Server must still be alive after the burst
        self.assertTrue(
            self.server_is_alive(),
            "Server process died during concurrent connection test",
        )


if __name__ == "__main__":
    unittest.main()
