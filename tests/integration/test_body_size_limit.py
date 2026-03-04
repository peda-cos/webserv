"""
Integration test: client_max_body_size enforcement.

Verifies that the server rejects request bodies that exceed the
configured limit with 413 Payload Too Large, while still accepting
bodies that fit within the limit.
"""

import sys
import os
import shutil
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from integration import WebservTestCase

TEST_DIR = "/tmp/webserv_test_bodylimit"
UPLOAD_DIR = os.path.join(TEST_DIR, "uploads")


class TestBodySizeLimit(WebservTestCase):
    config_fixture = "body_limit.conf"
    server_port = 8409

    def setUp(self):
        # Create the document root and the upload target directory
        # BEFORE starting the server.
        if os.path.exists(TEST_DIR):
            shutil.rmtree(TEST_DIR)
        os.makedirs(UPLOAD_DIR)

        super(TestBodySizeLimit, self).setUp()

    def tearDown(self):
        super(TestBodySizeLimit, self).tearDown()
        try:
            if os.path.exists(TEST_DIR):
                shutil.rmtree(TEST_DIR)
        except OSError:
            pass

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_oversized_body_rejected(self):
        """POST /upload with a body larger than 1024 bytes should return 413."""
        oversized_body = "A" * 2048
        status, headers, body = self.http_post(
            "/upload", oversized_body, "application/octet-stream"
        )
        self.assertEqual(status, 413)

    def test_small_body_accepted(self):
        """POST /upload with a body under 1024 bytes should NOT return 413."""
        small_body = "B" * 512
        status, headers, body = self.http_post(
            "/upload", small_body, "application/octet-stream"
        )
        self.assertNotEqual(status, 413)


if __name__ == "__main__":
    unittest.main()
