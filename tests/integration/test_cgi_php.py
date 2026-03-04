"""
Integration tests: PHP CGI execution.

Verifies that webserv correctly invokes php-cgi and returns its output.
The test is skipped if /usr/bin/php-cgi is not installed.
"""

import os
import sys
import stat
import shutil
import unittest

# Ensure the tests/ directory is on the import path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from integration import WebservTestCase

CGI_ROOT = "/tmp/webserv_test_cgi"
CGI_BIN = os.path.join(CGI_ROOT, "cgi-bin")
PHP_CGI = "/usr/bin/php-cgi"


class TestCgiPhp(WebservTestCase):
    """CGI execution of a PHP script that echoes the request method."""

    config_fixture = "cgi.conf"
    server_port = 8404

    def setUp(self):
        if not os.path.exists(PHP_CGI):
            self.skipTest("php-cgi not found at " + PHP_CGI)

        # Create document root and cgi-bin directory BEFORE starting server.
        if os.path.exists(CGI_ROOT):
            shutil.rmtree(CGI_ROOT)
        os.makedirs(CGI_BIN)

        # Minimal index.html for the root location
        with open(os.path.join(CGI_ROOT, "index.html"), "w") as f:
            f.write("<html><body>cgi test</body></html>\n")

        # PHP script that echoes REQUEST_METHOD
        hello_php = os.path.join(CGI_BIN, "hello.php")
        with open(hello_php, "w") as f:
            f.write("<?php\n")
            f.write('header("Content-Type: text/plain");\n')
            f.write('echo $_SERVER["REQUEST_METHOD"];\n')
            f.write("?>\n")
        os.chmod(hello_php, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP |
                 stat.S_IROTH | stat.S_IXOTH)

        super(TestCgiPhp, self).setUp()

    def tearDown(self):
        try:
            super(TestCgiPhp, self).tearDown()
        finally:
            if os.path.exists(CGI_ROOT):
                shutil.rmtree(CGI_ROOT)

    # ------------------------------------------------------------------ tests

    def test_php_echoes_method(self):
        status, headers, body = self.http_get("/cgi-bin/hello.php")
        self.assertEqual(status, 200, "Expected 200 OK, got %d" % status)
        self.assertIn(
            b"GET", body,
            "Expected body to contain 'GET', got: " + repr(body),
        )


if __name__ == "__main__":
    unittest.main()
