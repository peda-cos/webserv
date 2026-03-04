"""
Conformance tests: HTTP status codes.

Verifies that webserv returns the correct status code for every
condition listed in the project requirements table.  A single
comprehensive config is generated at runtime so that all scenarios
(static files, uploads, redirects, CGI, body limits) can be exercised
against one server instance on port 8420.
"""

import os
import sys
import stat
import shutil
import uuid
import tempfile
import subprocess
import time
import socket
import unittest

# Ensure the tests/ directory is on the import path so we can reuse
# the base-class helpers (http_get, http_post, etc.) without duplicating
# the raw-socket plumbing.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase, WEBSERV_BIN, SERVER_START_TIMEOUT, \
    SERVER_START_POLL_INTERVAL

# -------------------------------------------------------------------
# Paths used by the generated config
# -------------------------------------------------------------------
TEST_ROOT = "/tmp/webserv_conformance_status"
UPLOAD_DIR = os.path.join(TEST_ROOT, "uploads")
CGI_DIR = os.path.join(TEST_ROOT, "cgi-bin")
FORBIDDEN_DIR = os.path.join(TEST_ROOT, "forbidden_dir")
ERRORS_DIR = os.path.join(TEST_ROOT, "errors")
PYTHON3 = "/usr/bin/python3"

PORT = 8420


def _build_multipart(filename, data, boundary):
    """Build a minimal multipart/form-data body with one file field."""
    part = (
        "--" + boundary + "\r\n"
        'Content-Disposition: form-data; name="file"; '
        'filename="' + filename + '"\r\n'
        "Content-Type: application/octet-stream\r\n"
        "\r\n"
    )
    body = part.encode("utf-8") + data + b"\r\n"
    body += ("--" + boundary + "--\r\n").encode("utf-8")
    return body


class TestStatusCodes(WebservTestCase):
    """
    Trigger every required HTTP status code and assert the server
    returns the correct one.

    Because we need a single config that covers static files, uploads,
    redirects, CGI, and body-size limits we generate it on the fly in
    setUp() and override the base-class server-start logic.
    """

    # We override setUp entirely, but the base class checks these so
    # we satisfy the interface even though they are not used by the
    # base setUp (which we skip).
    config_fixture = "__unused__"
    server_host = "127.0.0.1"
    server_port = PORT

    # ----------------------------------------------------------------
    # Fixture setup / teardown
    # ----------------------------------------------------------------

    def setUp(self):
        # -- filesystem fixtures ----------------------------------------
        if os.path.exists(TEST_ROOT):
            shutil.rmtree(TEST_ROOT)

        os.makedirs(UPLOAD_DIR)
        os.makedirs(CGI_DIR)
        os.makedirs(FORBIDDEN_DIR)
        os.makedirs(ERRORS_DIR)

        # Normal index
        with open(os.path.join(TEST_ROOT, "index.html"), "w") as f:
            f.write("<html><body>OK</body></html>\n")

        # File that will be deleted
        self._deleteme = os.path.join(TEST_ROOT, "deleteme.txt")
        with open(self._deleteme, "w") as f:
            f.write("delete me\n")

        # Error page
        with open(os.path.join(ERRORS_DIR, "404.html"), "w") as f:
            f.write("<html><body>Custom 404</body></html>\n")

        # CGI: a valid hello script
        hello_py = os.path.join(CGI_DIR, "hello.py")
        with open(hello_py, "w") as f:
            f.write("#!/usr/bin/python3\n")
            f.write("import os\n")
            f.write('print("Content-Type: text/plain")\n')
            f.write('print("")\n')
            f.write('print("hello from cgi")\n')
        os.chmod(hello_py, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP
                 | stat.S_IROTH | stat.S_IXOTH)

        # CGI: a script that sleeps forever (for 504 test)
        slow_py = os.path.join(CGI_DIR, "slow.py")
        with open(slow_py, "w") as f:
            f.write("#!/usr/bin/python3\n")
            f.write("import time\n")
            f.write("time.sleep(999)\n")
        os.chmod(slow_py, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP
                 | stat.S_IROTH | stat.S_IXOTH)

        # -- config file -----------------------------------------------
        self._config = tempfile.NamedTemporaryFile(
            mode="w", suffix=".conf", prefix="webserv_conformance_",
            delete=False
        )
        self._config.write(
            "server {\n"
            "    listen 127.0.0.1:" + str(PORT) + ";\n"
            "\n"
            "    root " + TEST_ROOT + ";\n"
            "    index index.html;\n"
            "    client_max_body_size 1024;\n"
            "\n"
            "    error_page 404 /errors/404.html;\n"
            "\n"
            "    location /old-page {\n"
            "        return 301 /new-page;\n"
            "    }\n"
            "\n"
            "    location /temp-page {\n"
            "        return 302 /other-page;\n"
            "    }\n"
            "\n"
            "    location /upload {\n"
            "        limit_except POST;\n"
            "        upload_store " + UPLOAD_DIR + ";\n"
            "    }\n"
            "\n"
            "    location /cgi-bin {\n"
            "        cgi_pass .py " + PYTHON3 + ";\n"
            "        root " + CGI_DIR + ";\n"
            "    }\n"
            "\n"
            "    location /bad-cgi {\n"
            "        cgi_pass .py /usr/bin/nonexistent_interpreter_xyz;\n"
            "        root " + CGI_DIR + ";\n"
            "    }\n"
            "\n"
            "    location /forbidden_dir {\n"
            "        autoindex off;\n"
            "    }\n"
            "\n"
            "    location /restrict {\n"
            "        limit_except GET;\n"
            "    }\n"
            "\n"
            "    location / {\n"
            "        autoindex off;\n"
            "    }\n"
            "}\n"
        )
        self._config.close()

        # -- start server -----------------------------------------------
        webserv = os.path.abspath(WEBSERV_BIN)
        if not os.path.exists(webserv):
            self.skipTest("webserv binary not found at: " + webserv)

        self._server_proc = subprocess.Popen(
            [webserv, self._config.name],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        if not self._wait_for_server():
            self._kill_server()
            self.fail(
                "Server did not start within "
                + str(SERVER_START_TIMEOUT) + "s"
            )

    def tearDown(self):
        try:
            self._kill_server()
        finally:
            if os.path.exists(TEST_ROOT):
                shutil.rmtree(TEST_ROOT)
            if hasattr(self, "_config") and os.path.exists(self._config.name):
                os.unlink(self._config.name)

    # ----------------------------------------------------------------
    # Tests — one per required status code
    # ----------------------------------------------------------------

    def test_200_ok(self):
        """GET an existing file -> 200 OK."""
        status, headers, body = self.http_get("/index.html")
        self.assertEqual(status, 200, "Expected 200, got %d" % status)
        self.assertIn(b"OK", body)

    def test_201_created(self):
        """POST a multipart upload -> 201 Created."""
        boundary = "----Boundary" + uuid.uuid4().hex[:12]
        file_data = b"small upload content"
        body = _build_multipart("tiny.txt", file_data, boundary)
        content_type = "multipart/form-data; boundary=" + boundary

        status, headers, resp_body = self.http_post(
            "/upload", body.decode("latin-1"), content_type
        )
        self.assertEqual(status, 201, "Expected 201, got %d" % status)

    def test_204_no_content(self):
        """DELETE an existing file -> 204 No Content."""
        self.assertTrue(os.path.exists(self._deleteme))
        status, headers, body = self.http_delete("/deleteme.txt")
        self.assertEqual(status, 204, "Expected 204, got %d" % status)
        self.assertFalse(os.path.exists(self._deleteme))

    def test_301_moved(self):
        """GET a permanent-redirect path -> 301 Moved Permanently."""
        status, headers, body = self.http_get("/old-page")
        self.assertEqual(status, 301, "Expected 301, got %d" % status)
        self.assertIn("location", headers)
        self.assertIn("/new-page", headers["location"])

    def test_302_found(self):
        """GET a temporary-redirect path -> 302 Found."""
        status, headers, body = self.http_get("/temp-page")
        self.assertEqual(status, 302, "Expected 302, got %d" % status)
        self.assertIn("location", headers)
        self.assertIn("/other-page", headers["location"])

    def test_400_bad_request(self):
        """Send malformed HTTP -> 400 Bad Request."""
        status, headers, body = self.http_raw(
            "BROKEN GARBAGE\r\n\r\n"
        )
        self.assertEqual(status, 400, "Expected 400, got %d" % status)

    def test_403_forbidden(self):
        """GET a directory with no index and autoindex off -> 403 Forbidden."""
        status, headers, body = self.http_get("/forbidden_dir/")
        self.assertEqual(status, 403, "Expected 403, got %d" % status)

    def test_404_not_found(self):
        """GET a path that does not exist -> 404 Not Found."""
        status, headers, body = self.http_get("/this_does_not_exist.html")
        self.assertEqual(status, 404, "Expected 404, got %d" % status)

    def test_405_method_not_allowed(self):
        """Send a method that the location forbids -> 405 Method Not Allowed."""
        # /restrict only allows GET; a DELETE should be rejected.
        status, headers, body = self.http_delete("/restrict/something")
        self.assertEqual(status, 405, "Expected 405, got %d" % status)

    def test_413_payload_too_large(self):
        """POST a body larger than client_max_body_size -> 413."""
        oversized = "X" * 2048  # limit is 1024
        status, headers, body = self.http_post(
            "/upload", oversized, "application/octet-stream"
        )
        self.assertEqual(status, 413, "Expected 413, got %d" % status)

    def test_500_internal_error(self):
        """Trigger a broken CGI interpreter -> 500 Internal Server Error."""
        if not os.path.exists(PYTHON3):
            self.skipTest("python3 not found; cannot set up CGI fixture")
        # /bad-cgi maps .py to a nonexistent interpreter; the server
        # should fail the fork/exec and return 500.
        status, headers, body = self.http_get("/bad-cgi/hello.py")
        self.assertEqual(status, 500, "Expected 500, got %d" % status)

    def test_504_gateway_timeout(self):
        """CGI script that hangs forever -> 504 Gateway Timeout."""
        if not os.path.exists(PYTHON3):
            self.skipTest("python3 not available")

        # The base helper has a 5-second socket timeout.  The server's
        # CGI timeout should fire before that (typically 3-5 s).  We
        # give the socket extra room to receive the 504 response.
        old_timeout = 5.0
        # Temporarily increase the socket timeout so we don't time out
        # before the server sends the 504.
        saved = socket.getdefaulttimeout()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(30.0)
            sock.connect((self.server_host, self.server_port))

            request = (
                "GET /cgi-bin/slow.py HTTP/1.1\r\n"
                "Host: " + self.server_host + ":" + str(self.server_port) + "\r\n"
                "Connection: close\r\n"
                "\r\n"
            )
            sock.sendall(request.encode("utf-8"))

            response = b""
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    break
            sock.close()

            status, headers, body = self._parse_response(response)
            self.assertEqual(status, 504, "Expected 504, got %d" % status)
        finally:
            socket.setdefaulttimeout(saved)


if __name__ == "__main__":
    unittest.main()
