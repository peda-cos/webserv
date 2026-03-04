"""
Integration tests: file upload via POST multipart/form-data.

Verifies that webserv correctly receives a multipart upload, stores
the file on disk, and responds with 201 Created.
"""

import os
import sys
import shutil
import uuid
import unittest

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

UPLOAD_ROOT = "/tmp/webserv_test_upload"
UPLOAD_DIR = os.path.join(UPLOAD_ROOT, "uploads")


def _build_multipart(fields, boundary):
    """
    Build a multipart/form-data body from a list of field dicts.

    Each field dict:
        {"name": "...", "filename": "...", "content_type": "...", "data": b"..."}

    Returns the body as a plain string (suitable for http_post).
    """
    parts = []
    for field in fields:
        part = "--" + boundary + "\r\n"
        part += 'Content-Disposition: form-data; name="' + field["name"] + '"'
        if "filename" in field:
            part += '; filename="' + field["filename"] + '"'
        part += "\r\n"
        if "content_type" in field:
            part += "Content-Type: " + field["content_type"] + "\r\n"
        part += "\r\n"
        parts.append(part)

    # Assemble: each text part header + binary data + CRLF, then closing boundary
    body = b""
    for i, field in enumerate(fields):
        body += parts[i].encode("utf-8")
        if isinstance(field["data"], bytes):
            body += field["data"]
        else:
            body += field["data"].encode("utf-8")
        body += b"\r\n"
    body += ("--" + boundary + "--\r\n").encode("utf-8")
    return body


class TestPostUpload(WebservTestCase):
    """POST multipart/form-data file uploads."""

    config_fixture = "upload.conf"
    server_port = 8405

    def setUp(self):
        # Create document root and upload directory BEFORE starting server.
        if os.path.exists(UPLOAD_ROOT):
            shutil.rmtree(UPLOAD_ROOT)
        os.makedirs(UPLOAD_DIR)

        # A minimal index so the root location is valid
        with open(os.path.join(UPLOAD_ROOT, "index.html"), "w") as f:
            f.write("<html><body>upload test</body></html>\n")

        super(TestPostUpload, self).setUp()

    def tearDown(self):
        try:
            super(TestPostUpload, self).tearDown()
        finally:
            if os.path.exists(UPLOAD_ROOT):
                shutil.rmtree(UPLOAD_ROOT)

    # ------------------------------------------------------------------ tests

    def test_multipart_upload(self):
        boundary = "----WebservBoundary" + uuid.uuid4().hex[:16]
        file_content = b"This is the uploaded file content.\n"
        filename = "testfile.txt"

        body = _build_multipart(
            [
                {
                    "name": "file",
                    "filename": filename,
                    "content_type": "text/plain",
                    "data": file_content,
                }
            ],
            boundary,
        )

        content_type = "multipart/form-data; boundary=" + boundary
        # http_post expects body as str; convert bytes body via latin-1 to
        # preserve every byte value (multipart bodies are binary-safe this way).
        status, headers, resp_body = self.http_post(
            "/upload", body.decode("latin-1"), content_type
        )

        self.assertEqual(status, 201, "Expected 201 Created, got %d" % status)

        # Verify the file landed on disk.  The server may store it with the
        # original filename or a generated name — check for *any* new file.
        stored_files = os.listdir(UPLOAD_DIR)
        self.assertTrue(
            len(stored_files) > 0,
            "Expected at least one file in " + UPLOAD_DIR,
        )


if __name__ == "__main__":
    unittest.main()
