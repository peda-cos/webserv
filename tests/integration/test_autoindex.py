"""
Integration test: autoindex directory listing.

Verifies that when autoindex is enabled, a GET on a directory returns
an HTML listing containing the names of all files in that directory.
"""

import sys
import os
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

TEST_DIR = "/tmp/webserv_test_autoindex"


class TestAutoindex(WebservTestCase):
    config_fixture = "autoindex.conf"
    server_port = 8406

    def setUp(self):
        # Create the document root and seed it with test files BEFORE
        # the base class starts the server.
        if os.path.exists(TEST_DIR):
            shutil.rmtree(TEST_DIR)
        os.makedirs(TEST_DIR)

        for name in ("testfile1.txt", "testfile2.txt", "testfile3.html"):
            with open(os.path.join(TEST_DIR, name), "w") as f:
                f.write(name + " content\n")

        super(TestAutoindex, self).setUp()

    def tearDown(self):
        super(TestAutoindex, self).tearDown()
        try:
            if os.path.exists(TEST_DIR):
                shutil.rmtree(TEST_DIR)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_directory_listing(self):
        """GET / with autoindex on should return 200 and list all files."""
        status, headers, body = self.http_get("/")
        self.assertEqual(status, 200)

        body_text = body.decode("utf-8", errors="replace")
        self.assertIn("testfile1.txt", body_text)
        self.assertIn("testfile2.txt", body_text)
        self.assertIn("testfile3.html", body_text)


if __name__ == "__main__":
    unittest.main()
