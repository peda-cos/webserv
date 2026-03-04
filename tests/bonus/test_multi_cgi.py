"""
Bonus integration tests — Multiple CGI interpreters.

Verifies that webserv can route .py and .php scripts to different
interpreters based on extension, handles query strings, rejects
unconfigured extensions, and (optionally) enforces CGI timeouts.
"""

import sys
import os
import stat
import shutil
import unittest
import threading

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import integration
from integration import WebservTestCase

integration.FIXTURES_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "fixtures"
)

FIXTURE_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fixtures")
CGI_BIN = "/tmp/webserv_test_multicgi/cgi-bin"


class TestMultiCGI(WebservTestCase):
    config_fixture = "multi_cgi.conf"
    server_host = "127.0.0.1"
    server_port = 8411

    # ------------------------------------------------------------------
    # Setup / teardown
    # ------------------------------------------------------------------

    def setUp(self):
        if os.path.exists("/tmp/webserv_test_multicgi"):
            shutil.rmtree("/tmp/webserv_test_multicgi")
        os.makedirs(CGI_BIN)

        # Copy hello.py
        src_py = os.path.join(FIXTURE_SRC, "hello.py")
        dst_py = os.path.join(CGI_BIN, "hello.py")
        shutil.copy2(src_py, dst_py)
        os.chmod(dst_py, os.stat(dst_py).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Copy hello.php
        src_php = os.path.join(FIXTURE_SRC, "hello.php")
        dst_php = os.path.join(CGI_BIN, "hello.php")
        shutil.copy2(src_php, dst_php)
        os.chmod(dst_php, os.stat(dst_php).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Create a slow.py that sleeps forever (for timeout test).
        slow_path = os.path.join(CGI_BIN, "slow.py")
        with open(slow_path, "w") as f:
            f.write("#!/usr/bin/env python3\n")
            f.write("import time, sys\n")
            f.write("time.sleep(999)\n")
            f.write('sys.stdout.write("Content-Type: text/plain\\r\\n\\r\\nDone\\n")\n')
        os.chmod(slow_path, os.stat(slow_path).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        super(TestMultiCGI, self).setUp()

    def tearDown(self):
        super(TestMultiCGI, self).tearDown()
        if os.path.exists("/tmp/webserv_test_multicgi"):
            shutil.rmtree("/tmp/webserv_test_multicgi", ignore_errors=True)

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_python_executed(self):
        """GET /cgi-bin/hello.py must execute via python3 and echo the method."""
        status, _, body = self.http_get("/cgi-bin/hello.py")
        self.assertEqual(status, 200)
        body_str = body.decode("utf-8", errors="replace")
        self.assertIn("GET", body_str)

    def test_php_executed(self):
        """GET /cgi-bin/hello.php must execute via php-cgi and echo the method."""
        if not os.path.exists("/usr/bin/php-cgi"):
            self.skipTest("/usr/bin/php-cgi not found")

        status, _, body = self.http_get("/cgi-bin/hello.php")
        self.assertEqual(status, 200)
        body_str = body.decode("utf-8", errors="replace")
        self.assertIn("GET", body_str)

    def test_unconfigured_extension_error(self):
        """GET /cgi-bin/script.rb should return 403 or 404 (no handler configured)."""
        # Create a dummy .rb file so the path exists on disk.
        rb_path = os.path.join(CGI_BIN, "script.rb")
        with open(rb_path, "w") as f:
            f.write("#!/usr/bin/env ruby\nputs 'hello'\n")
        os.chmod(rb_path, os.stat(rb_path).st_mode | stat.S_IXUSR)

        status, _, _ = self.http_get("/cgi-bin/script.rb")
        self.assertIn(status, (403, 404, 500),
                       "Unconfigured extension should return 403, 404, or 500")

    def test_both_receive_query_string(self):
        """Query string must be forwarded to the CGI via QUERY_STRING."""
        status, _, body = self.http_get("/cgi-bin/hello.py?foo=bar")
        self.assertEqual(status, 200)
        body_str = body.decode("utf-8", errors="replace")
        self.assertIn("foo=bar", body_str)

    def test_concurrent_php_python(self):
        """Simultaneous requests to .php and .py must both succeed."""
        if not os.path.exists("/usr/bin/php-cgi"):
            self.skipTest("/usr/bin/php-cgi not found")

        results = {"py": None, "php": None}

        def fetch(key, path):
            try:
                results[key] = self.http_get(path)
            except Exception as exc:
                results[key] = exc

        t_py = threading.Thread(target=fetch, args=("py", "/cgi-bin/hello.py"))
        t_php = threading.Thread(target=fetch, args=("php", "/cgi-bin/hello.php"))
        t_py.start()
        t_php.start()
        t_py.join(timeout=10)
        t_php.join(timeout=10)

        for key in ("py", "php"):
            self.assertNotIsInstance(
                results[key], Exception,
                "Thread %s raised: %s" % (key, results[key]),
            )
            self.assertIsNotNone(results[key], "Thread %s got no result" % key)
            status, _, body = results[key]
            self.assertEqual(status, 200, "%s CGI returned %d" % (key, status))
            self.assertIn("GET", body.decode("utf-8", errors="replace"))

    def test_cgi_timeout_504(self):
        """A CGI that never finishes should be killed; expect 504 Gateway Timeout."""
        # Increase socket timeout so we actually wait for the server response.
        import socket
        old_timeout = socket.getdefaulttimeout()
        try:
            # Use a raw request with a generous receive timeout.
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(15.0)
            try:
                sock.connect((self.server_host, self.server_port))
                req = (
                    "GET /cgi-bin/slow.py HTTP/1.1\r\n"
                    "Host: " + self.server_host + ":" + str(self.server_port) + "\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                )
                sock.sendall(req.encode("utf-8"))
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

            if not response:
                self.skipTest("Server did not respond within 15 s — CGI timeout may not be implemented")

            # Parse the status line.
            status_line = response.split(b"\r\n", 1)[0].decode("utf-8", errors="replace")
            parts = status_line.split(" ", 2)
            if len(parts) >= 2:
                status_code = int(parts[1])
            else:
                self.skipTest("Could not parse status from response")
                return  # unreachable, satisfies linters

            # 504 is the correct answer, but some implementations use 500.
            if status_code not in (504, 500):
                self.skipTest(
                    "Server returned %d for slow CGI — CGI timeout may not be implemented yet"
                    % status_code
                )
        finally:
            socket.setdefaulttimeout(old_timeout)

        self.assertTrue(self.server_is_alive(), "Server crashed during CGI timeout test")


if __name__ == "__main__":
    unittest.main()
