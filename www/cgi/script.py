#!/usr/bin/python3
import os


# CGI-style headers section
print("Content-Type: text/html")
print("")

print("<html><body><h1>CGI Environment</h1>")

required_vars = [
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "GATEWAY_INTERFACE",
    "PATH_INFO",
    "QUERY_STRING",
    "REQUEST_METHOD",
    "REQUEST_URI",
    "SCRIPT_NAME",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
]

for key in required_vars:
    print(key + ": " + os.environ.get(key, "") + "<br>")

if os.environ.get("REQUEST_METHOD", "GET") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    post_data = os.read(0, content_length).decode("utf-8")
    print("<hr>POST body: " + post_data + "<br>")

print("</body></html>")

