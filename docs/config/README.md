# WebServ Configuration

## Overview

WebServ uses an NGINX-like configuration format. The configuration is loaded at startup and validated before the server begins accepting connections.

## Configuration File Location

By default, WebServ looks for `config/webserv.conf` in the working directory. You can specify a different path:

```bash
./webserv /path/to/config.conf
```

## Syntax

### Basic Structure

```
server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
}
```

### Directives

Directives end with a semicolon (`;`) and have the format:

```
directive_name argument1 argument2 ...;
```

### Blocks

Blocks use curly braces (`{ }`) and can contain nested directives:

```
server {
    listen 80;
    
    location / {
        index index.html;
    }
}
```

### Comments

Lines starting with `#` are comments:

```
# This is a comment
listen 80;  # This is also a comment
```

### Quoted Strings

Strings can be quoted with single or double quotes:

```
root "/var/www/html";
server_name 'example.com';
```

Escape sequences supported:
- `\\` - Backslash
- `\"` - Double quote
- `\'` - Single quote
- `\n` - Newline
- `\t` - Tab

## Directives

### Server Context

#### `listen`
Specifies the port (and optionally host) to listen on.

```
listen 80;
listen 127.0.0.1:8080;
listen [::]:8080;
```

#### `server_name`
Sets the server name.

```
server_name example.com www.example.com;
```

#### `root`
Sets the root directory for files.

```
root /var/www/html;
```

#### `client_max_body_size`
Sets the maximum allowed size of client request body.

```
client_max_body_size 10M;
client_max_body_size 1G;
```

Suffixes:
- `k` or `K` - Kilobytes
- `m` or `M` - Megabytes
- `g` or `G` - Gigabytes

### Location Context

#### `location`
Creates a location block for matching request paths.

```
location / {
    root /var/www/html;
}

location /api {
    allowed_methods GET POST;
}
```

#### `allowed_methods`
Specifies allowed HTTP methods.

```
allowed_methods GET POST DELETE;
```

Valid methods: `GET`, `POST`, `DELETE`, `PUT`, `HEAD`

#### `index`
Sets the default index file.

```
index index.html;
```

#### `autoindex`
Enables or disables directory listing.

```
autoindex on;
autoindex off;
```

#### `return`
Configures redirects.

```
return 301 /new-path;
return 302 http://example.com;
```

#### `upload_enabled`
Enables file uploads.

```
upload_enabled on;
```

#### `upload_dir`
Sets the directory for uploaded files.

```
upload_dir /var/www/uploads;
```

#### `cgi_map`
Maps file extensions to CGI interpreters.

```
cgi_map .php /usr/bin/php-cgi;
cgi_map .py /usr/bin/python;
```

## Inheritance

Some directives are inherited from server to location context:

- `client_max_body_size` - If not set in location, inherits from server

## Example Configuration

```
server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
    client_max_body_size 10M;
    
    location / {
        index index.html;
        autoindex off;
    }
    
    location /api {
        allowed_methods GET POST;
        cgi_map .php /usr/bin/php-cgi;
    }
    
    location /upload {
        upload_enabled on;
        upload_dir /var/www/uploads;
        allowed_methods POST;
    }
    
    error_page 404 /404.html;
    error_page 500 502 503 504 /50x.html;
}
```

## Error Handling

The configuration is validated at startup:
- Syntax errors cause immediate exit with error message
- Semantic errors (invalid ports, duplicate directives) are reported
- Server will not start if configuration is invalid

## Testing

Run the configuration tests:

```bash
./tests/config/LexerTest
./tests/config/ParserTest
./tests/config/ValidatorTest
./tests/config/IntegrationTest
```
