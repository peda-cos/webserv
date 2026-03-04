"""
Integration test: HTTP redirects (301 / 302).

Verifies that the server returns the correct status codes and Location
headers for routes configured with 'return 301 …' and 'return 302 …'.
"""

import sys
import os
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

TEST_DIR = "/tmp/webserv_test_redirect"


class TestRedirects(WebservTestCase):
    config_fixture = "redirect.conf"
    server_port = 8408

    def setUp(self):
        # Create the document root with a minimal index BEFORE starting
        # the server.
        if os.path.exists(TEST_DIR):
            shutil.rmtree(TEST_DIR)
        os.makedirs(TEST_DIR)

        with open(os.path.join(TEST_DIR, "index.html"), "w") as f:
            f.write("<html><body>redirect test root</body></html>\n")

        super(TestRedirects, self).setUp()

    def tearDown(self):
        super(TestRedirects, self).tearDown()
        try:
            if os.path.exists(TEST_DIR):
                shutil.rmtree(TEST_DIR)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_301_redirect(self):
        """GET /old-page should return 301 with Location pointing to /new-page."""
        status, headers, body = self.http_get("/old-page")
        self.assertEqual(status, 301)
        self.assertIn("location", headers)
        self.assertIn("/new-page", headers["location"])

    def test_302_redirect(self):
        """GET /temp-page should return 302 with Location pointing to /other-page."""
        status, headers, body = self.http_get("/temp-page")
        self.assertEqual(status, 302)
        self.assertIn("location", headers)
        self.assertIn("/other-page", headers["location"])


if __name__ == "__main__":
    unittest.main()
