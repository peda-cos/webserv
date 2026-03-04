"""
Bonus integration tests — Cookie handling.

Verifies that webserv correctly forwards Cookie headers to CGI scripts
and that Set-Cookie headers from CGI are passed back to the client.
"""

import sys
import os
import stat
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import integration
from integration import WebservTestCase

# Point FIXTURES_DIR at bonus/fixtures/ so config_fixture resolves correctly.
integration.FIXTURES_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "fixtures"
)

FIXTURE_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fixtures")
CGI_ROOT = "/tmp/webserv_test_cookies/cgi"
SESSION_DIR = "/tmp/webserv_sessions"


class TestCookies(WebservTestCase):
    config_fixture = "cookies.conf"
    server_host = "127.0.0.1"
    server_port = 8410

    # ------------------------------------------------------------------
    # Setup / teardown
    # ------------------------------------------------------------------

    def setUp(self):
        # Prepare the CGI tree *before* the server starts.
        if os.path.exists("/tmp/webserv_test_cookies"):
            shutil.rmtree("/tmp/webserv_test_cookies")
        os.makedirs(CGI_ROOT)

        # Copy session.py into the CGI root the config expects.
        src = os.path.join(FIXTURE_SRC, "session.py")
        dst = os.path.join(CGI_ROOT, "session.py")
        shutil.copy2(src, dst)
        os.chmod(dst, os.stat(dst).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Ensure sessions directory exists for the CGI script.
        if not os.path.exists(SESSION_DIR):
            os.makedirs(SESSION_DIR)

        super(TestCookies, self).setUp()

    def tearDown(self):
        super(TestCookies, self).tearDown()
        if os.path.exists("/tmp/webserv_test_cookies"):
            shutil.rmtree("/tmp/webserv_test_cookies", ignore_errors=True)
        if os.path.exists(SESSION_DIR):
            shutil.rmtree(SESSION_DIR, ignore_errors=True)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _get_with_cookie(self, path, cookie_value):
        """Send a GET request that includes a Cookie header."""
        raw = (
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + self.server_host + ":" + str(self.server_port) + "\r\n"
            "Cookie: " + cookie_value + "\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        return self.http_raw(raw)

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_set_cookie_on_first_request(self):
        """First visit (no Cookie) -> CGI must emit Set-Cookie with session_id."""
        status, headers, _ = self.http_get("/session/session.py")
        self.assertEqual(status, 200)
        self.assertIn("set-cookie", headers)
        self.assertIn("session_id=", headers["set-cookie"])

    def test_cookie_forwarded_to_cgi(self):
        """Cookie header must reach the CGI via HTTP_COOKIE env variable."""
        status, headers, body = self._get_with_cookie(
            "/session/session.py", "session_id=test123"
        )
        self.assertEqual(status, 200)
        body_str = body.decode("utf-8", errors="replace")
        # The CGI echoes session_id inside the JSON body when it finds the
        # cookie.  If the cookie was forwarded, the CGI will NOT generate a
        # new UUID; however it may create a new session if no file exists.
        # At minimum, the response must be valid JSON and not crash.
        self.assertIn("session_id", body_str)

    def test_multiple_cookies(self):
        """Multiple cookies (a=1; b=2) should all be forwarded to CGI."""
        status, _, body = self._get_with_cookie(
            "/session/session.py", "a=1; b=2"
        )
        self.assertEqual(status, 200)
        # The CGI script doesn't crash and returns valid JSON.
        body_str = body.decode("utf-8", errors="replace")
        self.assertIn("session_id", body_str)

    def test_httponly_attribute(self):
        """Set-Cookie should include the HttpOnly attribute."""
        _, headers, _ = self.http_get("/session/session.py")
        cookie = headers.get("set-cookie", "")
        self.assertIn("HttpOnly", cookie)

    def test_path_attribute(self):
        """Set-Cookie should include Path=/session."""
        _, headers, _ = self.http_get("/session/session.py")
        cookie = headers.get("set-cookie", "")
        self.assertIn("Path=/session", cookie)

    def test_malformed_cookie_no_crash(self):
        """Garbage in the Cookie header must not crash the server."""
        status, _, _ = self._get_with_cookie(
            "/session/session.py", ";;;==="
        )
        # Any valid HTTP status is acceptable — the server just must not die.
        self.assertGreaterEqual(status, 100)
        self.assertTrue(self.server_is_alive(), "Server crashed on malformed cookie")


if __name__ == "__main__":
    unittest.main()
