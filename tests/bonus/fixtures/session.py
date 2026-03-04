#!/usr/bin/env python3
"""
CGI session script — visit-counter using cookies.
On first visit: generates UUID session_id, sets Set-Cookie, returns visits=1.
On subsequent visits with valid cookie: increments visits, returns count.
"""
import os
import sys
import json
import uuid
import hashlib

# Simple in-memory session store (file-based for persistence across requests)
SESSION_DIR = "/tmp/webserv_sessions"

def get_session_file(session_id):
    safe = hashlib.md5(session_id.encode()).hexdigest()
    return os.path.join(SESSION_DIR, safe + ".json")

def load_session(session_id):
    path = get_session_file(session_id)
    if os.path.exists(path):
        with open(path, "r") as f:
            return json.load(f)
    return None

def save_session(session_id, data):
    if not os.path.exists(SESSION_DIR):
        os.makedirs(SESSION_DIR)
    path = get_session_file(session_id)
    with open(path, "w") as f:
        json.dump(data, f)

def parse_cookies(cookie_str):
    cookies = {}
    if cookie_str:
        for pair in cookie_str.split(";"):
            pair = pair.strip()
            if "=" in pair:
                k, v = pair.split("=", 1)
                cookies[k.strip()] = v.strip()
    return cookies

def main():
    cookie_header = os.environ.get("HTTP_COOKIE", "")
    cookies = parse_cookies(cookie_header)

    session_id = cookies.get("session_id", "")
    session_data = None
    new_session = False

    if session_id:
        session_data = load_session(session_id)

    if session_data is None:
        session_id = str(uuid.uuid4())
        session_data = {"visits": 0}
        new_session = True

    session_data["visits"] = session_data.get("visits", 0) + 1
    save_session(session_id, session_data)

    body = json.dumps({
        "session_id": session_id,
        "visits": session_data["visits"]
    })

    sys.stdout.write("Content-Type: application/json\r\n")
    if new_session:
        sys.stdout.write("Set-Cookie: session_id=" + session_id + "; Path=/session; HttpOnly\r\n")
    sys.stdout.write("Content-Length: " + str(len(body)) + "\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)

if __name__ == "__main__":
    main()
