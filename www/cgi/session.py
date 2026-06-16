#!/usr/bin/env python3
import os
import sys
import json
import uuid
import hashlib

SESSION_DIR = "/tmp/webserv_sessions"


def session_file(session_id):
    return os.path.join(SESSION_DIR, hashlib.md5(session_id.encode()).hexdigest() + ".json")


def parse_cookies(cookie_header):
    cookies = {}
    for part in cookie_header.split(";"):
        part = part.strip()
        if "=" in part:
            key, value = part.split("=", 1)
            cookies[key.strip()] = value.strip()
    return cookies


def main():
    if not os.path.isdir(SESSION_DIR):
        os.makedirs(SESSION_DIR)

    cookies = parse_cookies(os.environ.get("HTTP_COOKIE", ""))
    session_id = cookies.get("session_id", "")
    new_session = False
    visits = 0

    if session_id:
        try:
            with open(session_file(session_id), "r") as handle:
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
    with open(session_file(session_id), "w") as handle:
        json.dump({"visits": visits}, handle)

    body = {
        "session_id": session_id,
        "visits": visits,
        "html": "Session ID: " + session_id + "\nVisit Count: " + str(visits)
    }
    encoded = json.dumps(body)

    sys.stdout.write("Content-Type: application/json\r\n")
    if new_session:
        sys.stdout.write("Set-Cookie: session_id=" + session_id + "; Path=/; HttpOnly\r\n")
    sys.stdout.write("Content-Length: " + str(len(encoded)) + "\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(encoded)


if __name__ == "__main__":
    main()
