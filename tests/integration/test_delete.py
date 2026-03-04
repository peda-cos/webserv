"""
Integration tests: DELETE method.

Verifies that webserv deletes an existing file and returns 204,
and returns 404 for a file that does not exist.
"""

import os
import sys
import shutil
import unittest

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

STATIC_ROOT = "/tmp/webserv_test_static"


class TestDelete(WebservTestCase):
    """DELETE requests for existing and non-existing resources."""

    config_fixture = "single_server.conf"
    server_port = 8401

    def setUp(self):
        # Create document root and a file to be deleted BEFORE starting server.
        if os.path.exists(STATIC_ROOT):
            shutil.rmtree(STATIC_ROOT)
        os.makedirs(STATIC_ROOT)

        self._deleteme_path = os.path.join(STATIC_ROOT, "deleteme.txt")
        with open(self._deleteme_path, "w") as f:
            f.write("delete me\n")

        super(TestDelete, self).setUp()

    def tearDown(self):
        try:
            super(TestDelete, self).tearDown()
        finally:
            if os.path.exists(STATIC_ROOT):
                shutil.rmtree(STATIC_ROOT)

    # ------------------------------------------------------------------ tests

    def test_delete_existing_file(self):
        # Sanity check: file must exist before the request
        self.assertTrue(
            os.path.exists(self._deleteme_path),
            "Test file should exist before DELETE",
        )

        status, headers, body = self.http_delete("/deleteme.txt")
        self.assertEqual(status, 204, "Expected 204 No Content, got %d" % status)

        # The file must no longer exist on disk
        self.assertFalse(
            os.path.exists(self._deleteme_path),
            "File should have been removed after DELETE",
        )

    def test_delete_missing_file(self):
        status, headers, body = self.http_delete("/nonexistent.txt")
        self.assertEqual(status, 404, "Expected 404 Not Found, got %d" % status)


if __name__ == "__main__":
    unittest.main()
