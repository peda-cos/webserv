#!/usr/bin/env python3
import os
import sys
import json
import uuid
import hashlib

SESSION_DIR = "/tmp/webserv_sessions"

# Create session directory if it doesn't exist
if not os.path.isdir(SESSION_DIR):
    os.makedirs(SESSION_DIR)

# Parse cookies from HTTP_COOKIE environment variable
def parse_cookies(cookie_header):
    cookies = {}
    for part in cookie_header.split(";"):
        part = part.strip()
        if "=" in part:
            key, value = part.split("=", 1)
            cookies[key.strip()] = value.strip()
    return cookies

# Get cookie header and parse cookies
cookie_header = os.environ.get("HTTP_COOKIE", "")
cookies = parse_cookies(cookie_header)
session_id = cookies.get("session_id", "")

# Handle session logic
new_session = False
visits = 0

if session_id:
    session_file = os.path.join(SESSION_DIR, hashlib.md5(session_id.encode()).hexdigest() + ".json")
    try:
        with open(session_file, "r") as handle:
            data = json.load(handle)
            visits = int(data.get("visits", 0))
    except Exception:
        session_id = str(uuid.uuid4())
        new_session = True
        visits = 0
else:
    session_id = str(uuid.uuid4())
    new_session = True

visits += 1

# Save session data
session_file = os.path.join(SESSION_DIR, hashlib.md5(session_id.encode()).hexdigest() + ".json")
with open(session_file, "w") as handle:
    json.dump({"visits": visits}, handle)

# Prepare response
body = {
    "session_id": session_id,
    "visits": visits,
    "html": "Session ID: " + session_id + "\nVisit Count: " + str(visits)
}
encoded = json.dumps(body)

# Output HTTP headers
print("Content-Type: application/json")
if new_session:
    print("Set-Cookie: session_id=" + session_id + "; Path=/session; HttpOnly")
print("Content-Length: " + str(len(encoded)))
print("")

# Output response body
print(encoded)