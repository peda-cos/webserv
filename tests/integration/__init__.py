"""
Shared utilities for webserv integration tests.

Provides a base test class that manages server lifecycle:
- Spawns ./webserv with a given config fixture
- Waits for the server to accept connections
- Kills the server in teardown (even on failure)

Python stdlib only — no external dependencies.
"""

import os
import sys
import time
import signal
import socket
import subprocess
import unittest

# Path to the webserv binary (relative to tests/ directory)
WEBSERV_BIN = os.path.join(os.path.dirname(__file__), "..", "..", "webserv")
FIXTURES_DIR = os.path.join(os.path.dirname(__file__), "..", "fixtures")

# How long to wait for the server to start accepting connections (seconds)
SERVER_START_TIMEOUT = 5.0
SERVER_START_POLL_INTERVAL = 0.1


class WebservTestCase(unittest.TestCase):
    """
    Base class for integration tests that need a running webserv instance.

    Subclasses MUST set:
        config_fixture = "fixture_name.conf"   # file in tests/fixtures/
        server_host = "127.0.0.1"
        server_port = 8401                      # must match the fixture

    The server is started in setUp() and killed in tearDown() via try/finally.
    """

    config_fixture = None  # Override in subclass
    server_host = "127.0.0.1"
    server_port = 8401  # Override in subclass

    _server_proc = None

    def setUp(self):
        if self.config_fixture is None:
            self.fail("config_fixture not set on test class")

        config_path = os.path.join(FIXTURES_DIR, self.config_fixture)
        if not os.path.exists(config_path):
            self.fail("Config fixture not found: " + config_path)

        # Resolve webserv binary
        webserv = os.path.abspath(WEBSERV_BIN)
        if not os.path.exists(webserv):
            self.skipTest("webserv binary not found at: " + webserv)

        # Start server
        self._server_proc = subprocess.Popen(
            [webserv, config_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # Wait for server to accept connections
        if not self._wait_for_server():
            self._kill_server()
            self.fail(
                "Server did not start accepting connections within "
                + str(SERVER_START_TIMEOUT) + " seconds"
            )

    def tearDown(self):
        self._kill_server()

    def _wait_for_server(self):
        """Poll until the server accepts a TCP connection or timeout."""
        deadline = time.time() + SERVER_START_TIMEOUT
        while time.time() < deadline:
            # Check if process died
            if self._server_proc.poll() is not None:
                return False
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1.0)
                sock.connect((self.server_host, self.server_port))
                sock.close()
                return True
            except (socket.error, OSError):
                time.sleep(SERVER_START_POLL_INTERVAL)
        return False

    def _kill_server(self):
        """Kill the server process and reap it."""
        if self._server_proc is not None:
            try:
                self._server_proc.terminate()
                self._server_proc.wait(timeout=3)
            except Exception:
                try:
                    self._server_proc.kill()
                    self._server_proc.wait(timeout=2)
                except Exception:
                    pass
            self._server_proc = None

    def http_get(self, path, host=None, port=None):
        """
        Send a raw HTTP/1.1 GET request and return (status_code, headers_dict, body).
        Uses raw sockets — no http.client — for maximum control.
        """
        if host is None:
            host = self.server_host
        if port is None:
            port = self.server_port

        request = (
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + host + ":" + str(port) + "\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        return self._send_raw(host, port, request)

    def http_post(self, path, body, content_type="application/octet-stream",
                  host=None, port=None):
        """Send a raw HTTP/1.1 POST request."""
        if host is None:
            host = self.server_host
        if port is None:
            port = self.server_port

        request = (
            "POST " + path + " HTTP/1.1\r\n"
            "Host: " + host + ":" + str(port) + "\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + str(len(body)) + "\r\n"
            "Connection: close\r\n"
            "\r\n"
        ) + body
        return self._send_raw(host, port, request)

    def http_delete(self, path, host=None, port=None):
        """Send a raw HTTP/1.1 DELETE request."""
        if host is None:
            host = self.server_host
        if port is None:
            port = self.server_port

        request = (
            "DELETE " + path + " HTTP/1.1\r\n"
            "Host: " + host + ":" + str(port) + "\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        return self._send_raw(host, port, request)

    def http_raw(self, raw_request, host=None, port=None):
        """Send a completely raw request string."""
        if host is None:
            host = self.server_host
        if port is None:
            port = self.server_port
        return self._send_raw(host, port, raw_request)

    def _send_raw(self, host, port, request):
        """Send raw bytes and parse the HTTP response."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        try:
            sock.connect((host, port))
            if isinstance(request, str):
                sock.sendall(request.encode("utf-8"))
            else:
                sock.sendall(request)

            response = b""
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    break
        finally:
            sock.close()

        return self._parse_response(response)

    def _parse_response(self, raw):
        """Parse raw HTTP response into (status_code, headers_dict, body_bytes)."""
        if not raw:
            return (0, {}, b"")

        # Split headers and body
        header_end = raw.find(b"\r\n\r\n")
        if header_end == -1:
            header_end = len(raw)
            body = b""
        else:
            body = raw[header_end + 4:]

        header_section = raw[:header_end].decode("utf-8", errors="replace")
        lines = header_section.split("\r\n")

        # Parse status line
        status_code = 0
        if lines:
            parts = lines[0].split(" ", 2)
            if len(parts) >= 2:
                try:
                    status_code = int(parts[1])
                except ValueError:
                    pass

        # Parse headers
        headers = {}
        for line in lines[1:]:
            if ":" in line:
                key, value = line.split(":", 1)
                headers[key.strip().lower()] = value.strip()

        return (status_code, headers, body)

    def server_is_alive(self):
        """Check if the server process is still running."""
        if self._server_proc is None:
            return False
        return self._server_proc.poll() is None
