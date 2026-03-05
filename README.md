*jlacerda | peda-cos*

## Description

`webserv` is a C++98 HTTP/1.1 web server developed as part of the 42 school curriculum. It implements a subset of the HTTP protocol (RFC 7230) with support for:

- Static file serving (HTML, CSS, images)
- HTTP methods: GET, POST, DELETE
- CGI execution (PHP and Python scripts)
- File uploads with multipart/form-data
- Configurable error pages and directory listings
- NGINX-inspired configuration syntax

The server uses a single event loop with `poll()`/`epoll()`/`kqueue()` for I/O multiplexing, ensuring non-blocking operations throughout.

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

Stress test with siege:
```bash
siege -c 100 -t 30S http://localhost:8080
```

Or with the provided Python stress test:
```bash
python3 tests/stress.py
```

## Resources

### References

- [RFC 7230 - HTTP/1.1: Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1: Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [NGINX Documentation](https://nginx.org/en/docs/)
- [C++98 Standard (ISO/IEC 14882:1998)](https://www.open-std.org/jtc1/sc22/wg21/docs/wp/pdf/oct97/defects.pdf)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
