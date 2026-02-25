#include "http/HttpParser.hpp"

HttpParser::HttpParser(size_t max_body_size)
	: _state(REQUEST_LINE),
	  _body_remaining(0),
	  _header_bytes(0),
	  _max_body_size(max_body_size),
	  _error_code(0),
	  _seen_content_length(false)
{
}

ParseResult HttpParser::feed(std::string& rbuf)
{
	while (true) {
		if (_state == REQUEST_LINE) {
			ParseResult r = parse_request_line(rbuf);
			if (r != PARSE_INCOMPLETE) return r;
			if (_state == REQUEST_LINE) return PARSE_INCOMPLETE;
	} else if (_state == HEADERS) {
		ParseResult r = parse_headers(rbuf);
		if (r != PARSE_INCOMPLETE) return r;
		if (_state == HEADERS) return PARSE_INCOMPLETE;
} else if (_state == BODY) {
			ParseResult r = parse_body_cl(rbuf);
			if (r != PARSE_INCOMPLETE) return r;
			if (_state == BODY) return PARSE_INCOMPLETE;
		} else if (_state == CHUNK_SIZE) {
			ParseResult r = parse_chunk_size(rbuf);
			if (r != PARSE_INCOMPLETE) return r;
			if (_state == CHUNK_SIZE) return PARSE_INCOMPLETE;
		} else if (_state == CHUNK_DATA) {
			ParseResult r = parse_chunk_data(rbuf);
			if (r != PARSE_INCOMPLETE) return r;
			if (_state == CHUNK_DATA) return PARSE_INCOMPLETE;
		} else if (_state == TRAILERS) {
			ParseResult r = parse_trailers(rbuf);
			if (r != PARSE_INCOMPLETE) return r;
			if (_state == TRAILERS) return PARSE_INCOMPLETE;
		} else {
			return PARSE_COMPLETE;
		}
	}
}

static bool is_tchar(unsigned char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return true;
	const char* specials = "!#$%&'*+-.^_`|~";
	for (const char* p = specials; *p; ++p)
		if (c == (unsigned char)*p) return true;
	return false;
}

ParseResult HttpParser::parse_request_line(std::string& rbuf)
{
	std::string::size_type crlf = rbuf.find("\r\n");
	if (crlf == std::string::npos)
		return PARSE_INCOMPLETE;

	if (crlf > 8192) {
		_error_code = 414;
		return PARSE_ERROR;
	}

	std::string line = rbuf.substr(0, crlf);
	std::string::size_type len = line.size();
	std::string::size_type i = 0;

	// Parse METHOD: one or more tchar
	std::string::size_type method_start = i;
	while (i < len && is_tchar((unsigned char)line[i]))
		++i;
	if (i == method_start) {
		_error_code = 400;
		return PARSE_ERROR;
	}
	std::string method = line.substr(method_start, i - method_start);

	// Must have exactly one space
	if (i >= len || line[i] != ' ') {
		_error_code = 400;
		return PARSE_ERROR;
	}
	++i;

	// Parse TARGET: from here to next space
	std::string::size_type target_start = i;
	while (i < len && line[i] != ' ')
		++i;
	if (i == target_start) {
		_error_code = 400;
		return PARSE_ERROR;
	}
	std::string target = line.substr(target_start, i - target_start);
	if (target[0] != '/') {
		_error_code = 400;
		return PARSE_ERROR;
	}

	// Must have exactly one space
	if (i >= len || line[i] != ' ') {
		_error_code = 400;
		return PARSE_ERROR;
	}
	++i;

	// Parse VERSION: must be exactly "HTTP/1.0" or "HTTP/1.1"
	std::string version = line.substr(i);
	if (version != "HTTP/1.0" && version != "HTTP/1.1") {
		_error_code = 400;
		return PARSE_ERROR;
	}

	// Populate request
	_request.method = method;
	_request.target = target;
	_request.version = version;

	// Track bytes consumed (including CRLF) toward header section limit
	_header_bytes += crlf + 2;

	// Consume the line + CRLF from buffer
	rbuf.erase(0, crlf + 2);

	// Transition state
	_state = HEADERS;
	return PARSE_INCOMPLETE;
}

const HttpRequest& HttpParser::request() const
{
	return _request;
}

int HttpParser::error_code() const
{
	return _error_code;
}

void HttpParser::reset()
{
	_state = REQUEST_LINE;
	_request = HttpRequest();
	_body_remaining = 0;
	_header_bytes = 0;
	_error_code = 0;
	_seen_content_length = false;
}

ParseResult HttpParser::parse_headers(std::string& rbuf)
{
	while (true) {
		std::string::size_type crlf = rbuf.find("\r\n");
		if (crlf == std::string::npos)
			return PARSE_INCOMPLETE;

		// Check total header section size limit
		if (_header_bytes + crlf + 2 > 8192) {
			_error_code = 400;
			return PARSE_ERROR;
		}

		// Empty line = end of headers
		if (crlf == 0) {
			rbuf.erase(0, 2);
			if (_request.chunked)
				_state = CHUNK_SIZE;
			else if (_request.content_length > 0) {
				_state = BODY;
				_body_remaining = _request.content_length;
			} else {
				_state = DONE;
			}
			if (_state == DONE)
				return PARSE_COMPLETE;
			return PARSE_INCOMPLETE;
		}

		std::string line = rbuf.substr(0, crlf);

		// Reject obs-fold: line starting with SP or HTAB
		if (line[0] == ' ' || line[0] == '\t') {
			_error_code = 400;
			return PARSE_ERROR;
		}

		// Find ':'
		std::string::size_type colon = line.find(':');
		if (colon == std::string::npos) {
			_error_code = 400;
			return PARSE_ERROR;
		}

		// Reject empty field-name or whitespace before ':'
		if (colon == 0 || line[colon - 1] == ' ' || line[colon - 1] == '\t') {
			_error_code = 400;
			return PARSE_ERROR;
		}

		// Extract and validate field-name
		std::string name = line.substr(0, colon);
		for (std::string::size_type i = 0; i < name.size(); ++i) {
			if (!is_tchar((unsigned char)name[i])) {
				_error_code = 400;
				return PARSE_ERROR;
			}
		}

		// Normalize field-name to lowercase
		for (std::string::size_type i = 0; i < name.size(); ++i) {
			if (name[i] >= 'A' && name[i] <= 'Z')
				name[i] = name[i] + 32;
		}

		// Extract field-value (everything after ':')
		std::string value = line.substr(colon + 1);

		// Trim leading OWS
		{
			std::string::size_type start = 0;
			while (start < value.size() && (value[start] == ' ' || value[start] == '\t'))
				++start;
			value = value.substr(start);
		}

		// Trim trailing OWS
		{
			std::string::size_type end = value.size();
			while (end > 0 && (value[end - 1] == ' ' || value[end - 1] == '\t'))
				--end;
			value = value.substr(0, end);
		}

		// Scan for NUL bytes in field-value
		for (std::string::size_type i = 0; i < value.size(); ++i) {
			if (value[i] == '\0') {
				_error_code = 400;
				return PARSE_ERROR;
			}
		}

		// Process special headers
		if (name == "content-length") {
			// Parse value: only ASCII digits allowed
			if (value.empty()) {
				_error_code = 400;
				return PARSE_ERROR;
			}
			size_t new_cl = 0;
			for (std::string::size_type i = 0; i < value.size(); ++i) {
				unsigned char ch = (unsigned char)value[i];
				if (ch < '0' || ch > '9') {
					_error_code = 400;
					return PARSE_ERROR;
				}
				size_t digit = ch - '0';
				// Overflow check
				if (new_cl > ((size_t)-1 - digit) / 10) {
					_error_code = 400;
					return PARSE_ERROR;
				}
				new_cl = new_cl * 10 + digit;
			}
			// Duplicate CL with different value
			if (_seen_content_length && new_cl != _request.content_length) {
				_error_code = 400;
				return PARSE_ERROR;
			}
			_seen_content_length = true;
			_request.content_length = new_cl;
			// TE+CL conflict
			if (_request.chunked) {
				_error_code = 400;
				return PARSE_ERROR;
			}
		} else if (name == "transfer-encoding") {
			if (value == "chunked") {
				_request.chunked = true;
				// TE+CL conflict
				if (_seen_content_length) {
					_error_code = 400;
					return PARSE_ERROR;
				}
			} else {
				_error_code = 501;
				return PARSE_ERROR;
			}
		}

		// Store header
		_request.headers.push_back(std::make_pair(name, value));

		// Track bytes consumed
		_header_bytes += crlf + 2;

		// Consume line + CRLF from buffer
		rbuf.erase(0, crlf + 2);
	}
}

ParseResult HttpParser::parse_body_cl(std::string& rbuf)
{
	size_t to_consume = rbuf.size() < _body_remaining ? rbuf.size() : _body_remaining;
	if (to_consume == 0)
		return PARSE_INCOMPLETE;
	_request.body.append(rbuf.substr(0, to_consume));
	if (_request.body.size() > _max_body_size) {
		_error_code = 413;
		return PARSE_ERROR;
	}
	_body_remaining -= to_consume;
	rbuf.erase(0, to_consume);
	if (_body_remaining == 0) {
		_state = DONE;
		return PARSE_COMPLETE;
	}
	return PARSE_INCOMPLETE;
}

ParseResult HttpParser::parse_chunk_size(std::string& rbuf)
{
	std::string::size_type crlf = rbuf.find("\r\n");
	if (crlf == std::string::npos)
		return PARSE_INCOMPLETE;

	std::string line = rbuf.substr(0, crlf);
	std::string::size_type semi = line.find(';');
	std::string hex_part = (semi != std::string::npos) ? line.substr(0, semi) : line;

	if (hex_part.empty()) {
		_error_code = 400;
		return PARSE_ERROR;
	}
	for (std::string::size_type i = 0; i < hex_part.size(); ++i) {
		unsigned char c = (unsigned char)hex_part[i];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
			_error_code = 400;
			return PARSE_ERROR;
		}
	}

	size_t chunk_size = 0;
	for (std::string::size_type i = 0; i < hex_part.size(); ++i) {
		unsigned char c = (unsigned char)hex_part[i];
		size_t digit;
		if (c >= '0' && c <= '9')      digit = c - '0';
		else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
		else                           digit = c - 'A' + 10;
		if (chunk_size > ((size_t)-1 / 16)) {
			_error_code = 400;
			return PARSE_ERROR;
		}
		chunk_size = chunk_size * 16 + digit;
	}

	rbuf.erase(0, crlf + 2);

	if (chunk_size == 0) {
		_state = TRAILERS;
		return PARSE_INCOMPLETE;
	}
	_body_remaining = chunk_size;
	_state = CHUNK_DATA;
	return PARSE_INCOMPLETE;
}

ParseResult HttpParser::parse_chunk_data(std::string& rbuf)
{
	size_t to_consume = rbuf.size() < _body_remaining ? rbuf.size() : _body_remaining;
	if (to_consume == 0)
		return PARSE_INCOMPLETE;
	_request.body.append(rbuf.substr(0, to_consume));
	if (_request.body.size() > _max_body_size) {
		_error_code = 413;
		return PARSE_ERROR;
	}
	_body_remaining -= to_consume;
	rbuf.erase(0, to_consume);
	if (_body_remaining > 0)
		return PARSE_INCOMPLETE;
	// All chunk bytes consumed — now consume trailing CRLF
	if (rbuf.size() < 2)
		return PARSE_INCOMPLETE;
	if (rbuf[0] != '\r' || rbuf[1] != '\n') {
		_error_code = 400;
		return PARSE_ERROR;
	}
	rbuf.erase(0, 2);
	_state = CHUNK_SIZE;
	return PARSE_INCOMPLETE;
}

ParseResult HttpParser::parse_trailers(std::string& rbuf)
{
	while (true) {
		std::string::size_type crlf = rbuf.find("\r\n");
		if (crlf == std::string::npos)
			return PARSE_INCOMPLETE;
		if (crlf == 0) {
			rbuf.erase(0, 2);
			_state = DONE;
			return PARSE_COMPLETE;
		}
		std::string line = rbuf.substr(0, crlf);
		std::string::size_type colon = line.find(':');
		if (colon != std::string::npos && colon > 0) {
			std::string name = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			{
				std::string::size_type start = 0;
				while (start < value.size() && (value[start] == ' ' || value[start] == '\t'))
					++start;
				value = value.substr(start);
			}
			{
				std::string::size_type end = value.size();
				while (end > 0 && (value[end - 1] == ' ' || value[end - 1] == '\t'))
					--end;
				value = value.substr(0, end);
			}
			_request.headers.push_back(std::make_pair(name, value));
		}
		rbuf.erase(0, crlf + 2);
	}
}
