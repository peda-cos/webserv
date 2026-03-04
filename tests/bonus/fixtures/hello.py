#!/usr/bin/env python3
"""
Minimal CGI script that echoes REQUEST_METHOD and QUERY_STRING.
"""
import os
import sys

method = os.environ.get("REQUEST_METHOD", "UNKNOWN")
query = os.environ.get("QUERY_STRING", "")
cookie = os.environ.get("HTTP_COOKIE", "")

body = "method=" + method + "\nquery=" + query + "\ncookie=" + cookie + "\n"

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Content-Length: " + str(len(body)) + "\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
