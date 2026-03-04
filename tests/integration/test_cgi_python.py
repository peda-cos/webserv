"""
Integration tests: Python CGI execution.

Verifies that webserv correctly invokes a Python CGI script and returns
well-formed JSON output.  The test is skipped if /usr/bin/python3 is
not installed.
"""

import os
import sys
import stat
import json
import shutil
import unittest

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

CGI_ROOT = "/tmp/webserv_test_cgi"
CGI_BIN = os.path.join(CGI_ROOT, "cgi-bin")
PYTHON3 = "/usr/bin/python3"


class TestCgiPython(WebservTestCase):
    """CGI execution of a Python script that returns JSON."""

    config_fixture = "cgi.conf"
    server_port = 8404

    def setUp(self):
        if not os.path.exists(PYTHON3):
            self.skipTest("python3 not found at " + PYTHON3)

        # Create document root and cgi-bin directory BEFORE starting server.
        if os.path.exists(CGI_ROOT):
            shutil.rmtree(CGI_ROOT)
        os.makedirs(CGI_BIN)

        # Minimal index.html for the root location
        with open(os.path.join(CGI_ROOT, "index.html"), "w") as f:
            f.write("<html><body>cgi test</body></html>\n")

        # Python CGI script that returns JSON with the request method
        hello_py = os.path.join(CGI_BIN, "hello.py")
        with open(hello_py, "w") as f:
            f.write("#!/usr/bin/python3\n")
            f.write("import os, json\n")
            f.write('print("Content-Type: application/json")\n')
            f.write('print("")\n')
            f.write('print(json.dumps({"method": os.environ.get("REQUEST_METHOD", "UNKNOWN")}))\n')
        os.chmod(hello_py, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP |
                 stat.S_IROTH | stat.S_IXOTH)

        super(TestCgiPython, self).setUp()

    def tearDown(self):
        try:
            super(TestCgiPython, self).tearDown()
        finally:
            if os.path.exists(CGI_ROOT):
                shutil.rmtree(CGI_ROOT)

    # ------------------------------------------------------------------ tests

    def test_python_returns_json(self):
        status, headers, body = self.http_get("/cgi-bin/hello.py")
        self.assertEqual(status, 200, "Expected 200 OK, got %d" % status)

        # Content-Type must indicate JSON
        ct = headers.get("content-type", "")
        self.assertIn(
            "application/json", ct,
            "Expected application/json content-type, got: " + ct,
        )

        # Body must be valid JSON with the expected key
        try:
            data = json.loads(body)
        except (ValueError, TypeError) as exc:
            self.fail("Response body is not valid JSON: " + str(exc))

        self.assertEqual(
            data.get("method"), "GET",
            "Expected method 'GET' in JSON, got: " + repr(data),
        )


if __name__ == "__main__":
    unittest.main()
