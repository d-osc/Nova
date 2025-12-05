/**
 * HTTP Headers Benchmark
 *
 * Tests performance with multiple response headers
 */

import * as http from 'http';

const PORT = 3003;

const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'X-Custom-Header-1': 'value1',
        'X-Custom-Header-2': 'value2',
        'X-Custom-Header-3': 'value3',
        'X-Request-ID': Math.random().toString(36).substring(7),
        'Cache-Control': 'no-cache, no-store, must-revalidate',
        'Pragma': 'no-cache',
        'Expires': '0'
    });
    res.end('Headers test');
});

server.listen(PORT, () => {
    console.log(`HTTP Headers server listening on port ${PORT}`);
    console.log(`Test with: wrk -t4 -c100 -d30s http://127.0.0.1:${PORT}/`);
});
