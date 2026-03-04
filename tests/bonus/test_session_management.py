"""
Bonus integration tests — Session management via cookies.

Verifies that the session.py CGI script (and the server's cookie
pass-through) correctly implement session persistence, visit counting,
and concurrent session isolation.
"""

import sys
import os
import re
import json
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
CGI_ROOT = "/tmp/webserv_test_cookies/cgi"
SESSION_DIR = "/tmp/webserv_sessions"

# Regex for a UUID-v4 (8-4-4-4-12 hex).
UUID_RE = re.compile(
    r"[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}",
    re.IGNORECASE,
)


class TestSessionManagement(WebservTestCase):
    config_fixture = "cookies.conf"
    server_host = "127.0.0.1"
    server_port = 8410

    # ------------------------------------------------------------------
    # Setup / teardown
    # ------------------------------------------------------------------

    def setUp(self):
        for d in ("/tmp/webserv_test_cookies", SESSION_DIR):
            if os.path.exists(d):
                shutil.rmtree(d)
        os.makedirs(CGI_ROOT)
        os.makedirs(SESSION_DIR)

        src = os.path.join(FIXTURE_SRC, "session.py")
        dst = os.path.join(CGI_ROOT, "session.py")
        shutil.copy2(src, dst)
        os.chmod(dst, os.stat(dst).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        super(TestSessionManagement, self).setUp()

    def tearDown(self):
        super(TestSessionManagement, self).tearDown()
        for d in ("/tmp/webserv_test_cookies", SESSION_DIR):
            if os.path.exists(d):
                shutil.rmtree(d, ignore_errors=True)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _get_with_cookie(self, path, cookie_value):
        raw = (
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + self.server_host + ":" + str(self.server_port) + "\r\n"
            "Cookie: " + cookie_value + "\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        return self.http_raw(raw)

    def _extract_session_id_from_set_cookie(self, headers):
        """Return the session_id value from the Set-Cookie header, or None."""
        cookie = headers.get("set-cookie", "")
        m = re.search(r"session_id=([^;]+)", cookie)
        return m.group(1) if m else None

    def _parse_body_json(self, body):
        return json.loads(body.decode("utf-8", errors="replace"))

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_session_id_generated(self):
        """First request should produce a UUID session_id in Set-Cookie."""
        status, headers, _ = self.http_get("/session/session.py")
        self.assertEqual(status, 200)
        sid = self._extract_session_id_from_set_cookie(headers)
        self.assertIsNotNone(sid, "No session_id found in Set-Cookie header")
        self.assertTrue(UUID_RE.match(sid), "session_id is not a valid UUID-v4: " + str(sid))

    def test_session_persisted(self):
        """Two requests with the same session cookie return the same session_id."""
        # 1st request — get a session.
        status1, headers1, body1 = self.http_get("/session/session.py")
        self.assertEqual(status1, 200)
        sid = self._extract_session_id_from_set_cookie(headers1)
        self.assertIsNotNone(sid)

        # 2nd request — reuse the session cookie.
        status2, _, body2 = self._get_with_cookie(
            "/session/session.py", "session_id=" + sid
        )
        self.assertEqual(status2, 200)
        data2 = self._parse_body_json(body2)
        self.assertEqual(data2["session_id"], sid)

    def test_concurrent_distinct_sessions(self):
        """Two concurrent requests without cookies each get a different session_id."""
        results = [None, None]

        def fetch(index):
            try:
                s, h, b = self.http_get("/session/session.py")
                results[index] = (s, h, b)
            except Exception as exc:
                results[index] = exc

        t0 = threading.Thread(target=fetch, args=(0,))
        t1 = threading.Thread(target=fetch, args=(1,))
        t0.start()
        t1.start()
        t0.join(timeout=10)
        t1.join(timeout=10)

        for i in (0, 1):
            self.assertNotIsInstance(results[i], Exception, "Thread %d raised: %s" % (i, results[i]))
            self.assertIsNotNone(results[i], "Thread %d got no result" % i)

        sid0 = self._extract_session_id_from_set_cookie(results[0][1])
        sid1 = self._extract_session_id_from_set_cookie(results[1][1])
        self.assertIsNotNone(sid0)
        self.assertIsNotNone(sid1)
        self.assertNotEqual(sid0, sid1, "Concurrent requests must receive different session IDs")

    def test_visit_counter_increments(self):
        """Repeated requests with the same cookie should increment visits."""
        # 1st request — create session.
        _, headers, body1 = self.http_get("/session/session.py")
        sid = self._extract_session_id_from_set_cookie(headers)
        self.assertIsNotNone(sid)
        data1 = self._parse_body_json(body1)
        self.assertEqual(data1["visits"], 1)

        # 2nd request.
        _, _, body2 = self._get_with_cookie("/session/session.py", "session_id=" + sid)
        data2 = self._parse_body_json(body2)
        self.assertEqual(data2["visits"], 2)

        # 3rd request.
        _, _, body3 = self._get_with_cookie("/session/session.py", "session_id=" + sid)
        data3 = self._parse_body_json(body3)
        self.assertEqual(data3["visits"], 3)

    def test_invalid_session_id_handled(self):
        """A completely fake session_id must not crash the server."""
        status, headers, body = self._get_with_cookie(
            "/session/session.py", "session_id=FAKE_INVALID"
        )
        # The CGI should either create a new session (200 + Set-Cookie) or
        # reject the request (4xx).  Either way the server must survive.
        self.assertIn(status, (200, 400))
        self.assertTrue(self.server_is_alive(), "Server crashed on invalid session_id")

        if status == 200:
            # The CGI is expected to issue a fresh session.
            data = self._parse_body_json(body)
            self.assertIn("session_id", data)
            self.assertNotEqual(data["session_id"], "FAKE_INVALID")


if __name__ == "__main__":
    unittest.main()
