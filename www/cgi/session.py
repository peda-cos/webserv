#!/usr/bin/python3
import os
import sys
import uuid
import pathlib

# Directory for sessions
SESSION_DIR = pathlib.Path("/tmp/webserv_sessions")
SESSION_DIR.mkdir(parents=True, exist_ok=True)

# 1. Get Cookie from environment
cookie_header = os.environ.get("HTTP_COOKIE", "")
session_id = None

# Parse cookies
cookies = {}
if cookie_header:
    parts = cookie_header.split(";")
    for p in parts:
        if "=" in p:
            k, v = p.strip().split("=", 1)
            cookies[k] = v

session_id = cookies.get("session_id")
visits = 0

# 2. Check or create session
if session_id:
    session_file = SESSION_DIR / f"session_{session_id}"
    if session_file.exists():
        with open(session_file, "r") as f:
            try:
                visits = int(f.read().strip())
            except ValueError:
                visits = 0
    else:
        # Invalid session id, treat as new
        session_id = str(uuid.uuid4())
        visits = 0
else:
    # New session
    session_id = str(uuid.uuid4())
    visits = 0

visits += 1

# 3. Save session
session_file = SESSION_DIR / f"session_{session_id}"
with open(session_file, "w") as f:
    f.write(str(visits))

# 4. Output headers
print("Content-Type: text/html")
# HttpOnly prevents JavaScript access, Path=/ restricts scope
print(f"Set-Cookie: session_id={session_id}; Path=/; HttpOnly")
print("")

# 5. Output body
print("<html>")
print("<head><title>Webserv Session Test</title></head>")
print("<body>")
print(f"<h1>Session ID: {session_id}</h1>")
print(f"<h2>Visit Count: {visits}</h2>")
print("<p>Refresh the page to see the counter increment!</p>")
print("<p><a href='/'>Go Home</a></p>")
print("</body>")
print("</html>")
