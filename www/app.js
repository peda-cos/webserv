/**
 * WebServ Test Interface Architect
 */
const App = {
    Config: {
        host: '127.0.0.1',
        port: '8080',
        timeout: 5000,
        baseUrl: function() {
            return `http://${this.host}:${this.port}`;
        }
    },
    
    Storage: {
        saveConfig() {
            localStorage.setItem('webserv_config', JSON.stringify({
                host: document.getElementById('config-host').value,
                port: document.getElementById('config-port').value,
                timeout: document.getElementById('config-timeout').value,
                theme: document.documentElement.classList.contains('dark-theme') ? 'dark' : 'light'
            }));
        },
        loadConfig() {
            try {
                const conf = JSON.parse(localStorage.getItem('webserv_config'));
                if (conf) {
                    if(conf.host) document.getElementById('config-host').value = conf.host;
                    if(conf.port) document.getElementById('config-port').value = conf.port;
                    if(conf.timeout) document.getElementById('config-timeout').value = conf.timeout;
                    if(conf.theme === 'dark') {
                        document.documentElement.classList.add('dark-theme');
                        document.getElementById('btn-theme-toggle').textContent = '☀️';
                    }
                }
            } catch (e) {}
            App.Config.host = document.getElementById('config-host').value;
            App.Config.port = document.getElementById('config-port').value;
            App.Config.timeout = parseInt(document.getElementById('config-timeout').value) * 1000;
        }
    },

    Scenarios: [
        // --- CORE REQUIREMENTS (RFC 7231 HTTP Methods) ---
        { id: 1, name: '✅ GET / Index (200)', method: 'GET', path: '/', expectStatus: 200, validate: { hasBody: true, contains: ['webserv', 'html'], contentType: 'text/html' }, status: 'pending' },
        { id: 2, name: '✅ GET Health Check', method: 'GET', path: '/health-check.html', expectStatus: 200, validate: { contentType: 'text/html', hasBody: true, notEmpty: true, containsHtml: true, contains: ['Health Check', 'Operational'] }, status: 'pending' },
        { id: 3, name: '❌ 404 Not Found', method: 'GET', path: '/not-found-xyz.html', expectStatus: 404, validate: { hasBody: true, containsHtml: true }, status: 'pending' },
        { id: 4, name: '⛔ 405 Method Not Allowed', method: 'DELETE', path: '/', expectStatus: 405, validate: { hasBody: true, containsHtml: true, notContains: 'success' }, status: 'pending' },
        { id: 5, name: '📂 Autoindex Enabled (/imagens/)', method: 'GET', path: '/imagens/', expectStatus: 200, validate: { contentType: 'text/html', containsHtml: true, contains: ['href', 'imagens'] }, status: 'pending' },
        
        // --- POST/UPLOAD TESTS ---
        { id: 6, name: '📤 POST Upload (201)', method: 'POST', path: '/upload', expectStatus: 201, body: 'Test file content', contentType: 'text/plain', validate: { hasBody: true, notEmpty: true, containsHtml: true }, status: 'pending' },
        { id: 7, name: '📤 POST Empty Body', method: 'POST', path: '/upload', expectStatus: 201, body: '', contentType: 'text/plain', validate: { hasBody: true, notEmpty: true }, status: 'pending' },
        
        // --- DELETE TESTS ---
        { id: 8, name: '🗑️ DELETE Resource', method: 'DELETE', path: '/imagens/delete-target.txt', expectStatus: 204, validate: { noBody: true }, status: 'pending' },
        { id: 9, name: '⛔ DELETE Not Allowed', method: 'DELETE', path: '/', expectStatus: 405, validate: { hasBody: true, containsHtml: true }, status: 'pending' },
        
        // --- HEAD REQUEST (RFC 7231) ---
        { id: 10, name: '🐸 HEAD Request (no body)', method: 'HEAD', path: '/health-check.html', expectStatus: 200, validate: { noBody: true, hasHeader: 'content-type' }, status: 'pending' },
        
        // --- CGI EXECUTION (Must have real output) ---
        { id: 11, name: '⚙️ CGI GET Execute Script', method: 'GET', path: '/cgi/script.py', expectStatus: 200, validate: { hasBody: true, contentType: 'text/html', notEmpty: true, contains: ['CGI Environment', 'REQUEST_METHOD: GET'] }, status: 'pending' },
        { id: 12, name: '⚙️ CGI PATH_INFO + Query', method: 'GET', path: '/cgi/script.py/extra/path?x=1', expectStatus: 200, validate: { hasBody: true, contentType: 'text/html', contains: ['SCRIPT_NAME: /cgi/script.py', 'PATH_INFO: /extra/path', 'QUERY_STRING: x=1'] }, status: 'pending' },
        { id: 22, name: '⚙️ CGI QUERY_STRING Only', method: 'GET', path: '/cgi/script.py?name=webserv&suite=cgi', expectStatus: 200, validate: { hasBody: true, contentType: 'text/html', contains: ['PATH_INFO: <br>', 'QUERY_STRING: name=webserv&suite=cgi'] }, status: 'pending' },
        { id: 23, name: '⚙️ CGI POST Form Data', method: 'POST', path: '/cgi/script.py', expectStatus: 200, body: 'name=webserv&value=test', contentType: 'application/x-www-form-urlencoded', validate: { hasBody: true, notEmpty: true, contains: ['REQUEST_METHOD: POST', 'POST body: name=webserv&value=test'] }, status: 'pending' },
        { id: 24, name: '⚙️ CGI HEAD (No Body)', method: 'HEAD', path: '/cgi/script.py', expectStatus: 200, validate: { noBody: true, hasHeader: 'content-type' }, status: 'pending' },
        { id: 25, name: '⚙️ CGI DELETE Execute', method: 'DELETE', path: '/cgi/script.py', expectStatus: 200, validate: { hasBody: true, contentType: 'text/html', contains: 'REQUEST_METHOD: DELETE' }, status: 'pending' },
        { id: 26, name: '⚙️ CGI Script Missing (404)', method: 'GET', path: '/cgi/not-found.py', expectStatus: 404, validate: { hasBody: true, notEmpty: true, contains: 'Not Found' }, status: 'pending' },
        { id: 27, name: '⚙️ CGI Directory Request (200 Empty)', method: 'GET', path: '/cgi/', expectStatus: 200, validate: { noBody: true, contentType: 'application/octet-stream' }, status: 'pending' },
        { id: 28, name: '⚙️ CGI Timeout (504)', method: 'GET', path: '/cgi/timeout.py', expectStatus: 504, validate: { hasBody: true, notEmpty: true, contains: 'CGI execution timed out' }, status: 'pending' },
        { id: 29, name: '⚙️ CGI Script Failure (500)', method: 'GET', path: '/cgi/fail.py', expectStatus: 500, validate: { hasBody: true, notEmpty: true, contains: ['CGI script exited with code'] }, status: 'pending' },
        { id: 30, name: '⭐ BONUS CGI PHP Without Handler', method: 'GET', path: '/cgi/script.php', expectStatus: 404, validate: { hasBody: true, notEmpty: true, contains: '404' }, status: 'pending' },
        { id: 31, name: '⭐ BONUS CGI Perl Execute', method: 'GET', path: '/cgi/script.pl', expectStatus: 200, validate: { hasBody: true, contentType: 'text/html', notEmpty: true, contains: ['Perl CGI Environment', 'REQUEST_METHOD: GET'] }, status: 'pending' },
        { id: 32, name: '⭐ BONUS CGI Unknown Extension', method: 'GET', path: '/cgi/script.rb', expectStatus: 404, validate: { hasBody: true, notEmpty: true, contains: '404' }, status: 'pending' },
        
        // --- EDGE CASES & LIMITS ---
        { id: 13, name: '⚠️ Large Body >5MB (413)', method: 'POST', path: '/upload', expectStatus: 413, body: 'X'.repeat(5500000), contentType: 'text/plain', validate: { containsHtml: true }, status: 'pending' },
        { id: 14, name: '⚠️ Empty POST Body Response', method: 'POST', path: '/upload', expectStatus: 201, body: '', contentType: 'text/plain', validate: { hasBody: true, notEmpty: true }, status: 'pending' },
        { id: 15, name: '⚠️ Timeout Test (5s)', method: 'GET', path: '/timeout', expectStatus: 408, validate: { hasBody: true, contentType: 'text/html' }, status: 'pending' },
        
        // --- MALFORMED/SECURITY ---
        { id: 16, name: '🚫 Null Byte Injection (%00)', method: 'GET', path: '/%00', expectStatus: 400, validate: { hasBody: true, containsHtml: true }, status: 'pending' },
        { id: 17, name: '🚫 Path Traversal (..)', method: 'GET', path: '/../etc/passwd', expectStatus: 400, validate: { hasBody: true, containsHtml: true }, status: 'pending' },
        
        // --- HTTP HEADERS VALIDATION ---
        { id: 18, name: '📋 Custom User-Agent Header', method: 'GET', path: '/', headers: { 'User-Agent': 'WebServ-Tester/1.0' }, expectStatus: 200, validate: { hasBody: true, containsHtml: true, contentType: 'text/html' }, status: 'pending' },
        { id: 19, name: '📋 Host Header Required', method: 'GET', path: '/', headers: { 'Host': '127.0.0.1:8080' }, expectStatus: 200, validate: { hasBody: true, containsHtml: true, contentType: 'text/html' }, status: 'pending' },
        
        // --- BONUS & ADVANCED ---
        { id: 20, name: '🍪 Cookies Support (Set-Cookie)', method: 'GET', path: '/', headers: { 'Cookie': 'session=abc123' }, expectStatus: 200, validate: { hasBody: true, hasHeader: 'set-cookie', contains: ['cookie'] }, status: 'pending' },
        { id: 21, name: '↪️ Redirect (301/302)', method: 'GET', path: '/redirect', expectStatus: [301, 302], validate: { hasHeader: 'location' }, status: 'pending' }
    ],

    Utils: {
        BODY_METHODS: ['POST', 'PUT', 'PATCH', 'DELETE'],

        supportsRequestBody(method) {
            return this.BODY_METHODS.includes(method);
        },

        hasRequestBody(method, body) {
            return body !== null && body !== undefined && this.supportsRequestBody(method);
        }
    },

    Executor: {
        async send(method, path, headers = {}, body = null) {
            const url = App.Config.baseUrl() + path;
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), App.Config.timeout);

            let fetchOptions = {
                method: method,
                headers: headers,
                signal: controller.signal,
                // no-cors mode might hide status code (returns 0), we use default cors and rely on server or local bypass
                mode: 'cors' 
            };

            if (App.Utils.hasRequestBody(method, body)) {
                fetchOptions.body = body;
            }

            const start = performance.now();
            try {
                // Some endpoints might return CORS errors if server doesn't send Access-Control headers.
                // In local dev `file://` to `localhost`, browsers might complain.
                // We catch it and show gracefully.
                const response = await fetch(url, fetchOptions);
                const end = performance.now();
                clearTimeout(timeoutId);
                
                let responseText = '';
                let isBinary = false;
                
                // We only read body if not HEAD
                if (method !== 'HEAD') {
                    const buffer = await response.arrayBuffer();
                    // Check if seems binary
                    const uint8Arr = new Uint8Array(buffer.slice(0, 1024));
                    isBinary = uint8Arr.some(b => b < 32 && b !== 9 && b !== 10 && b !== 13);
                    
                    if (isBinary) {
                        responseText = `<Binary Data: ${buffer.byteLength} bytes>`;
                    } else {
                        const decoder = new TextDecoder('utf-8', {fatal: false});
                        responseText = decoder.decode(buffer);
                    }
                }

                // Parse Headers map
                const respHeaders = {};
                response.headers.forEach((val, key) => { respHeaders[key] = val; });

                // Reconstruction of the raw request payload for validation
                let rawRequest = `${method} ${path} HTTP/1.1\nHost: ${App.Config.host}:${App.Config.port}\n`;
                for (const [k, v] of Object.entries(headers)) {
                    rawRequest += `${k}: ${v}\n`;
                }
                rawRequest += `\n`;
                if (App.Utils.hasRequestBody(method, body)) {
                    rawRequest += body;
                }

                return {
                    success: true,
                    status: response.status,
                    statusText: response.statusText || this._getStatusText(response.status),
                    headers: respHeaders,
                    body: responseText,
                    rawRequest: rawRequest,
                    isBinary: isBinary,
                    time: Math.round(end - start),
                    url: response.url
                };
            } catch (error) {
                const end = performance.now();
                clearTimeout(timeoutId);
                let errStatus = 0;
                let errMsg = error.message;

                if (error.name === 'AbortError') {
                    errStatus = 408;
                    errMsg = 'Request Timeout';
                } else if (error.message.includes('Failed to fetch')) {
                    // Can be CORS or Connection Refused
                    errMsg = 'Connection Error / CORS Blocked';
                }

                return {
                    success: false,
                    status: errStatus,
                    statusText: errMsg,
                    headers: {},
                    body: error.stack,
                    rawRequest: `${method} ${path} HTTP/1.1\nHost: ${App.Config.host}:${App.Config.port}\n...(Request dropped)`,
                    time: Math.round(end - start),
                    url: url,
                    error: error
                };
            }
        },

        validateResponse(response, scenario) {
            const checks = [];
            
            // Check main status code
            let statusOk = false;
            if (Array.isArray(scenario.expectStatus)) {
                statusOk = scenario.expectStatus.includes(response.status);
            } else {
                statusOk = response.status === scenario.expectStatus;
            }

            checks.push({
                name: 'HTTP Status Code',
                passed: statusOk,
                message: statusOk ? `${response.status} ✓` : `Got ${response.status}, expected ${scenario.expectStatus}`
            });

            if (!statusOk) {
                return { pass: false, reason: `Status ${response.status} != Expected ${scenario.expectStatus}`, details: checks };
            }

            // Additional validations if provided
            if (scenario.validate) {
                const v = scenario.validate;
                
                // Check body presence/absence
                if (v.hasBody !== undefined) {
                    const hasBody = response.body && response.body.trim().length > 0;
                    checks.push({
                        name: 'Response Has Body',
                        passed: hasBody,
                        message: hasBody ? `Body present (${response.body.length} bytes)` : 'Expected response body but got empty'
                    });
                    if (!hasBody) {
                        return { pass: false, reason: 'Expected response body but got empty', details: checks };
                    }
                }
                
                if (v.noBody !== undefined) {
                    const hasNoBody = !response.body || response.body.trim().length === 0;
                    checks.push({
                        name: 'Response Has No Body',
                        passed: hasNoBody,
                        message: hasNoBody ? 'Body correctly empty' : `Expected no body but got ${response.body.length} bytes`
                    });
                    if (!hasNoBody) {
                        return { pass: false, reason: `HEAD/204 should not have body (got ${response.body.length} bytes)`, details: checks };
                    }
                }
                
                if (v.notEmpty !== undefined) {
                    const notEmpty = response.body && response.body.trim().length > 0;
                    checks.push({
                        name: 'Response Body Not Empty',
                        passed: notEmpty,
                        message: notEmpty ? 'Body is not empty ✓' : 'Response body is empty'
                    });
                    if (!notEmpty) {
                        return { pass: false, reason: 'Response body is empty', details: checks };
                    }
                }
                
                // Content-Type validation (single or array)
                if (v.contentType !== undefined) {
                    const expected = Array.isArray(v.contentType) ? v.contentType : [v.contentType];
                    const actual = response.headers['content-type'] ? response.headers['content-type'].split(';')[0].trim() : '';
                    const contentTypeok = expected.some(type => actual.includes(type));
                    checks.push({
                        name: 'Content-Type',
                        passed: contentTypeok,
                        message: contentTypeok ? `${actual} ✓` : `${actual} not in [${expected.join(', ')}]`
                    });
                    if (!contentTypeok) {
                        return { pass: false, reason: `Content-Type ${actual} not in [${expected.join(', ')}]`, details: checks };
                    }
                }
                
                // Check for specific header presence
                if (v.hasHeader !== undefined) {
                    const headers = Array.isArray(v.hasHeader) ? v.hasHeader : [v.hasHeader];
                    for (const header of headers) {
                        const hasHeader = !!response.headers[header.toLowerCase()];
                        checks.push({
                            name: `Header: ${header}`,
                            passed: hasHeader,
                            message: hasHeader ? `${response.headers[header.toLowerCase()]} ✓` : `Missing required header`
                        });
                        if (!hasHeader) {
                            return { pass: false, reason: `Missing required header: ${header}`, details: checks };
                        }
                    }
                }
                
                // Check for specific content strings
                if (v.contains !== undefined) {
                    const searches = Array.isArray(v.contains) ? v.contains : [v.contains];
                    for (const search of searches) {
                        const found = response.body && response.body.toLowerCase().includes(search.toLowerCase());
                        checks.push({
                            name: `Contains: "${search}"`,
                            passed: found,
                            message: found ? 'Found ✓' : 'Not found in body'
                        });
                        if (!found) {
                            return { pass: false, reason: `Body missing expected content: "${search}"`, details: checks };
                        }
                    }
                }
                
                // Check that content is NOT present
                if (v.notContains !== undefined) {
                    const searches = Array.isArray(v.notContains) ? v.notContains : [v.notContains];
                    for (const search of searches) {
                        const notFound = !response.body || !response.body.toLowerCase().includes(search.toLowerCase());
                        checks.push({
                            name: `Not Contains: "${search}"`,
                            passed: notFound,
                            message: notFound ? 'Not found ✓' : 'Unexpectedly found in body'
                        });
                        if (!notFound) {
                            return { pass: false, reason: `Body contains unexpected content: "${search}"`, details: checks };
                        }
                    }
                }
                
                // Check if HTML content (has HTML tags)
                if (v.containsHtml !== undefined) {
                    const isHtml = response.body && /<[a-z][a-z0-9]*\b/i.test(response.body);
                    checks.push({
                        name: 'Contains HTML Tags',
                        passed: isHtml,
                        message: isHtml ? 'HTML tags detected ✓' : 'Response is not valid HTML'
                    });
                    if (!isHtml) {
                        return { pass: false, reason: 'Response is not valid HTML (missing tags)', details: checks };
                    }
                }
            }

            return { pass: true, reason: 'All validations passed ✓', details: checks };
        },
        
        _getStatusText(code) {
            const statusCodes = {
                200: 'OK', 201: 'Created', 204: 'No Content',
                301: 'Moved Permanently', 302: 'Found', 304: 'Not Modified',
                400: 'Bad Request', 403: 'Forbidden', 404: 'Not Found', 
                405: 'Method Not Allowed', 408: 'Request Timeout', 413: 'Payload Too Large',
                500: 'Internal Server Error', 501: 'Not Implemented', 505: 'HTTP Version Not Supported'
            };
            return statusCodes[code] || 'Unknown';
        }
    },

    UI: {
        renderScenarios() {
            const list = document.getElementById('scenarios-list');
            list.innerHTML = '';
            
            // Calculate stats
            let stats = { pending: 0, pass: 0, fail: 0 };
            App.Scenarios.forEach(s => {
                if (s.status === 'pending' || s.status === 'loading') stats.pending++;
                else if (s.status === 'pass') stats.pass++;
                else if (s.status === 'fail') stats.fail++;
            });
            
            // Update stats display
            document.getElementById('stat-pending').textContent = stats.pending;
            document.getElementById('stat-pass').textContent = stats.pass;
            document.getElementById('stat-fail').textContent = stats.fail;
            
            const statsPanel = document.getElementById('stats-panel');
            if (stats.pass > 0 || stats.fail > 0) {
                statsPanel.style.display = 'flex';
            }
            
            App.Scenarios.forEach((s, index) => {
                const item = document.createElement('div');
                item.className = `scenario-item`;
                item.id = `sc-item-${s.id}`;
                
                let statusHtml = `<span class="scenario-status status-pending">⏳ Pending</span>`;
                let reasonHtml = '';
                let detailsHtml = '';
                if (s.status === 'pass') {
                    statusHtml = `<span class="scenario-status status-pass">✅ Pass</span>`;
                    reasonHtml = `<div style="font-size: 0.75rem; color: var(--success);">✓ ${s.validationReason || 'Success'}</div>`;
                    if (s.validationDetails && s.validationDetails.length > 0) {
                        detailsHtml = `<div style="margin-top: 6px; padding: 6px; background: rgba(76, 175, 80, 0.05); border-left: 3px solid var(--success); border-radius: 3px; font-size: 0.7rem;">`;
                        s.validationDetails.forEach(check => {
                            detailsHtml += `<div style="color: var(--success); margin: 2px 0;">✓ ${check.name}</div>`;
                        });
                        detailsHtml += `</div>`;
                    }
                } else if (s.status === 'fail') {
                    statusHtml = `<span class="scenario-status status-fail">❌ Fail</span>`;
                    reasonHtml = `<div style="font-size: 0.75rem; color: var(--error);">✗ ${s.validationReason || 'Status ' + s.actualStatus}</div>`;
                    if (s.validationDetails && s.validationDetails.length > 0) {
                        detailsHtml = `<div style="margin-top: 6px; padding: 6px; background: rgba(244, 67, 54, 0.05); border-left: 3px solid var(--error); border-radius: 3px; font-size: 0.7rem;">`;
                        s.validationDetails.forEach(check => {
                            const icon = check.passed ? '✓' : '✗';
                            const color = check.passed ? 'var(--success)' : 'var(--error)';
                            detailsHtml += `<div style="color: ${color}; margin: 2px 0;">${icon} ${check.name}: ${check.message || 'Check'}</div>`;
                        });
                        detailsHtml += `</div>`;
                    }
                } else if (s.status === 'loading') {
                    statusHtml = `<span class="scenario-status status-pending">⏳ Running...</span>`;
                }

                item.innerHTML = `
                    <div class="scenario-header">
                        <span class="scenario-title">
                            <span class="method-badge method-${s.method}">${s.method}</span>
                            ${s.name}
                        </span>
                        ${statusHtml}
                    </div>
                    <div class="scenario-details">
                        <span>Path: <code>${s.path}</code></span>
                        <span>Expects: <b>${Array.isArray(s.expectStatus) ? s.expectStatus.join(' or ') : s.expectStatus}</b></span>
                        ${s.actualStatus ? `<span style="color: var(--text-secondary);">Got: <b>${s.actualStatus}</b></span>` : ''}
                    </div>
                    ${reasonHtml}
                    ${detailsHtml}
                    <div class="scenario-actions">
                        <button onclick="window.Interface.runScenario(${index})">▶ Run</button>
                        <button onclick="window.Interface.loadScenarioToBuilder(${index})">📝 Load</button>
                    </div>
                `;
                list.appendChild(item);
            });
        },

        renderResponseViewer(response) {
            const container = document.getElementById('response-container');
            document.getElementById('resp-target-url').textContent = response.url;
            
            let statusClass = 'status-0xx';
            if(response.status >= 200 && response.status < 300) statusClass = 'status-2xx';
            else if(response.status >= 300 && response.status < 400) statusClass = 'status-3xx';
            else if(response.status >= 400 && response.status < 500) statusClass = 'status-4xx';
            else if(response.status >= 500) statusClass = 'status-5xx';

            let headersHtml = Object.entries(response.headers)
                .map(([k, v]) => `<b>${k}:</b> ${v}`)
                .join('\n') || 'No headers received';

            let bodySizeDetails = '';
            if (response.headers['content-length']) {
                bodySizeDetails = `Size: ${response.headers['content-length']} B`;
            } else if (response.body) {
                bodySizeDetails = `Length: ${response.body.length} chars`;
            }

            let contentType = response.headers['content-type'] || '';
            let bodyPreviewHtml = '';
            
            // XSS protection and preview handling
            if (contentType.includes('text/html') && !response.isBinary && response.body.length > 0) {
                // Render inside iframe for basic preview sandbox
                const blob = new Blob([response.body], {type: 'text/html'});
                const blobUrl = URL.createObjectURL(blob);
                bodyPreviewHtml = `<iframe src="${blobUrl}" class="html-preview" sandbox="allow-same-origin"></iframe>`;
            } else {
                // Escape text
                const escaped = response.body
                    .replace(/&/g, "&amp;")
                    .replace(/</g, "&lt;")
                    .replace(/>/g, "&gt;");
                bodyPreviewHtml = escaped || '<i>(Empty body)</i>';
            }
            
            if (response.body && response.body.length > 500000 && !response.isBinary) {
                 bodyPreviewHtml = `<i>(Body too large to preview fully, truncated to 500KB)</i>\n\n` + response.body.substring(0, 500000).replace(/</g, "&lt;");
            }
            
            // Generate validation info summary
            let validationInfo = '';
            if (response.headers['content-type']) {
                validationInfo += `<span style="font-size: 0.8rem; color: var(--success);">✓ Content-Type: ${response.headers['content-type']}</span><br>`;
            }
            if (response.body && response.body.length > 0) {
                validationInfo += `<span style="font-size: 0.8rem; color: var(--success);">✓ Response body present (${response.body.length} bytes)</span><br>`;
            }
            if (!response.body || response.body.length === 0) {
                validationInfo += `<span style="font-size: 0.8rem; color: var(--warning);">⚠ No response body</span><br>`;
            }

            container.innerHTML = `
                <div class="response-status-line ${statusClass}">
                    <div class="status-code">HTTP/1.1 ${response.status} ${response.statusText}</div>
                    <div class="response-meta">
                        <span class="meta-item">⏱️ ${response.time} ms</span>
                        <span class="meta-item">📦 ${bodySizeDetails || 'No Body'}</span>
                    </div>
                </div>

                <div>
                    <div style="font-size: 0.85rem; font-weight: bold; margin-bottom: 5px; color: var(--text-secondary);">Raw Request</div>
                    <div class="response-headers" style="background:var(--code-bg); border-color:var(--code-border); color:var(--code-text);">${(response.rawRequest||'').replace(/</g, '&lt;')}</div>
                </div>

                <div>
                    <div style="font-size: 0.85rem; font-weight: bold; margin-bottom: 5px; color: var(--text-secondary);">Response Headers</div>
                    <div class="response-headers">${headersHtml}</div>
                </div>
                
                <div style="padding: 8px; background: linear-gradient(135deg, rgba(31, 111, 235, 0.05), rgba(56, 139, 253, 0.05)); border-radius: 4px; margin-bottom: 10px; border-left: 3px solid var(--accent);">
                    <div style="font-size: 0.85rem; font-weight: bold; color: var(--text-secondary); margin-bottom: 5px;">📊 Response Metadata</div>
                    ${validationInfo}
                </div>

                <div class="response-body">
                    <div class="response-body-toolbar">
                        <span><b>Body</b> Preview ${contentType ? `(${contentType})` : ''}</span>
                    </div>
                    <pre class="response-body-content">${bodyPreviewHtml}</pre>
                </div>
            `;
        }
    }
};

// Interface Exposed to DOM Events
window.Interface = {
    init() {
        App.Storage.loadConfig();
        App.UI.renderScenarios();
        
        // Theme Toggle
        document.getElementById('btn-theme-toggle').addEventListener('click', (e) => {
            const html = document.documentElement;
            if (html.classList.contains('dark-theme')) {
                html.classList.remove('dark-theme');
                e.target.textContent = '🌙';
            } else {
                html.classList.add('dark-theme');
                e.target.textContent = '☀️';
            }
            App.Storage.saveConfig();
        });

        // Config Save Listener
        document.getElementById('btn-save-config').addEventListener('click', () => {
            App.Storage.saveConfig();
            App.Storage.loadConfig();
            alert('Configuration Saved!');
        });

        // Run All Listener
        document.getElementById('btn-run-all').addEventListener('click', async () => {
            document.getElementById('btn-run-all').disabled = true;
            for (let i = 0; i < App.Scenarios.length; i++) {
                await this.runScenario(i);
                // small delay between reqs
                await new Promise(r => setTimeout(r, 200));
            }
            document.getElementById('btn-run-all').disabled = false;
        });
        
        // Clear Listener
        document.getElementById('btn-clear-scenarios').addEventListener('click', () => {
            App.Scenarios.forEach(s => {
                s.status = 'pending';
                s.actualStatus = null;
            });
            App.UI.renderScenarios();
        });
        
        // Method Switcher => Show/hide Body tab
        document.getElementById('req-method').addEventListener('change', (e) => {
            const m = e.target.value;
            const tabBody = document.getElementById('tab-body');
            const bodyContent = document.getElementById('content-body');

            if (App.Utils.supportsRequestBody(m)) {
                if (tabBody) tabBody.style.display = 'block';
                if (bodyContent) bodyContent.style.display = 'block';
            } else {
                if (tabBody && tabBody.classList.contains('active')) {
                    this.switchTab('headers');
                }
                if (tabBody) tabBody.style.display = 'none';
                if (bodyContent) bodyContent.style.display = 'none';
            }
        });
        
        // trigger change once
        document.getElementById('req-method').dispatchEvent(new Event('change'));
    },

    switchTab(tabName) {
        document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));

        if (tabName === 'headers') {
            document.getElementById('content-headers').classList.add('active');
        } else {
            document.getElementById('content-body').classList.add('active');
        }
    },

    addHeaderRow() {
        const list = document.getElementById('req-headers-list');
        const row = document.createElement('div');
        row.className = 'header-row';
        row.innerHTML = `
            <input type="text" placeholder="Header Name" class="hdr-name">
            <input type="text" placeholder="Value" class="hdr-val">
            <button type="button" onclick="this.parentElement.remove()">×</button>
        `;
        list.appendChild(row);
    },

    getBuilderData() {
        const method = document.getElementById('req-method').value;
        const path = document.getElementById('req-path').value;
        
        let headers = {};
        document.querySelectorAll('.header-row').forEach(row => {
            const name = row.querySelector('.hdr-name').value.trim();
            const val = row.querySelector('.hdr-val').value.trim();
            if (name) headers[name] = val;
        });

        let body = null;
        if (App.Utils.supportsRequestBody(method)) {
            body = document.getElementById('req-body').value;
            const type = document.querySelector('input[name="bodyType"]:checked').value;
            if(type === 'json' && !headers['Content-Type']) headers['Content-Type'] = 'application/json';
            if(type === 'form' && !headers['Content-Type']) headers['Content-Type'] = 'application/x-www-form-urlencoded';
            if(type === 'plain' && !headers['Content-Type']) headers['Content-Type'] = 'text/plain';
        }

        return { method, path, headers, body };
    },

    async sendFreeRequest() {
        const req = this.getBuilderData();
        document.getElementById('response-container').innerHTML = `<div class="empty-state">⏳ Loading...</div>`;
        
        const response = await App.Executor.send(req.method, req.path, req.headers, req.body);
        App.UI.renderResponseViewer(response);
    },

    async runScenario(index) {
        const sc = App.Scenarios[index];
        
        // Visual update to pending/loading
        sc.status = 'loading';
        App.UI.renderScenarios();

        let headers = sc.headers ? { ...sc.headers } : {};
        if (sc.contentType) headers['Content-Type'] = sc.contentType;

        const response = await App.Executor.send(sc.method, sc.path, headers, sc.body);
        
        sc.actualStatus = response.status;
        const validation = App.Executor.validateResponse(response, sc);
        
        if (validation.pass) {
            sc.status = 'pass';
            sc.validationReason = 'PASS';
        } else {
            sc.status = 'fail';
            sc.validationReason = validation.reason;
        }
        
        // Store validation details for display
        if (validation.details) {
            sc.validationDetails = validation.details;
        }

        // Render specific scenario result
        App.UI.renderScenarios();
        
        // Show result in viewer
        App.UI.renderResponseViewer(response);
    },

    loadScenarioToBuilder(index) {
        const sc = App.Scenarios[index];
        document.getElementById('req-method').value = sc.method;
        document.getElementById('req-path').value = sc.path;
        
        // clean headers
        document.getElementById('req-headers-list').innerHTML = '';
        if(sc.contentType) {
            const list = document.getElementById('req-headers-list');
            const row = document.createElement('div');
            row.className = 'header-row';
            row.innerHTML = `
                <input type="text" class="hdr-name" value="Content-Type">
                <input type="text" class="hdr-val" value="${sc.contentType}">
                <button type="button" onclick="this.parentElement.remove()">×</button>
            `;
            list.appendChild(row);
        }

        if (sc.body !== undefined) {
            document.getElementById('req-body').value = sc.body;
            let btype = 'plain';
            if(sc.contentType === 'application/json') btype = 'json';
            else if(sc.contentType === 'application/x-www-form-urlencoded') btype = 'form';
            document.querySelector(`input[name="bodyType"][value="${btype}"]`).checked = true;
        } else {
            document.getElementById('req-body').value = '';
        }

        // Trigger method change to fix tabs visibility
        document.getElementById('req-method').dispatchEvent(new Event('change'));
    }
};

// Bootstrap
document.addEventListener('DOMContentLoaded', () => {
    window.Interface.init();
});
