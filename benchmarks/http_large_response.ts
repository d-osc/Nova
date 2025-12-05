/**
 * HTTP Large Response Benchmark
 *
 * Tests performance with larger response payloads (10KB)
 */

import * as http from 'http';

const PORT = 3004;

// Generate 10KB response
const LARGE_RESPONSE = 'x'.repeat(10 * 1024);

const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Content-Length': LARGE_RESPONSE.length
    });
    res.end(LARGE_RESPONSE);
});

server.listen(PORT, () => {
    console.log(`HTTP Large Response server listening on port ${PORT}`);
    console.log(`Response size: ${LARGE_RESPONSE.length} bytes`);
    console.log(`Test with: wrk -t4 -c100 -d30s http://127.0.0.1:${PORT}/`);
});
