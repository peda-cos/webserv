"""
Integration test: custom error pages.

Verifies that the server serves a custom 404 error page when configured
via the error_page directive, instead of a generic default.
"""

import sys
import os
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

TEST_DIR = "/tmp/webserv_test_errors"


class TestErrorPages(WebservTestCase):
    config_fixture = "error_pages.conf"
    server_port = 8407

    def setUp(self):
        # Create the document root and custom error page BEFORE the
        # server is started.
        if os.path.exists(TEST_DIR):
            shutil.rmtree(TEST_DIR)
        os.makedirs(os.path.join(TEST_DIR, "errors"))

        with open(os.path.join(TEST_DIR, "errors", "404.html"), "w") as f:
            f.write("<h1>Custom 404</h1>\n")

        super(TestErrorPages, self).setUp()

    def tearDown(self):
        super(TestErrorPages, self).tearDown()
        try:
            if os.path.exists(TEST_DIR):
                shutil.rmtree(TEST_DIR)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_custom_404_page(self):
        """GET /nonexistent should return 404 with our custom page body."""
        status, headers, body = self.http_get("/nonexistent")
        self.assertEqual(status, 404)

        body_text = body.decode("utf-8", errors="replace")
        self.assertIn("Custom 404", body_text)


if __name__ == "__main__":
    unittest.main()
