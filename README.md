*This project has been created as part of the 42 curriculum by jlacerda, lpaula-n, peda-cos*

## Description

`webserv` is a C++98 HTTP/1.1 web server developed as part of the 42 school curriculum. It implements a subset of the HTTP protocol (RFC 7230) with support for:

- Static file serving (HTML, CSS, images)
- HTTP methods: GET, POST, DELETE
- CGI execution (PHP and Python scripts)
- File uploads with multipart/form-data
- Configurable error pages and directory listings
- NGINX-inspired configuration syntax

The server uses a single event loop with `poll()` for I/O multiplexing, ensuring non-blocking operations throughout.

## Instructions

### Compilation

```bash
make
```

For a full rebuild:
```bash
make re
```

### Execution

Run the server with the default configuration:

```bash
./webserv config/default.conf
```

Or with a custom configuration file:

```bash
./webserv path/to/your.conf
```

### Testing

Unit tests (C++98):
```bash
cd tests && make unit
```

Integration and conformance tests (requires running server):
```bash
cd tests && make integration conformance stress bonus
```

Stress test with siege:
```bash
siege -c 100 -t 30S http://localhost:8080
```

### Testing Interface (Visual Web UI)

A browser-based test interface is provided at `http://localhost:8080` with 21 pre-configured scenarios covering GET, POST, DELETE, HEAD requests, status codes, errors, and edge cases.

## Resources

### References

- [RFC 7230 - HTTP/1.1: Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1: Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://tools.ietf.org/html/rfc3875)
- [NGINX Documentation](https://nginx.org/en/docs/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### AI Usage

- Understanding RFC specifications and NGINX behavior for HTTP parsing and response formatting
- Debugging non-blocking I/O patterns and edge cases in the event loop
- Writing and reviewing test cases for conformance and stress testing
- Refactoring for C++98 compliance and eliminating modern C++ patterns

All code was written, reviewed, and tested by the team. AI suggestions were adapted and verified against the subject requirements and actual server behavior.
