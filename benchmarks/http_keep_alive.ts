/**
 * HTTP Keep-Alive Benchmark
 *
 * Tests connection reuse with HTTP/1.1 keep-alive
 */

import * as http from 'http';

const PORT = 3002;
let requestCount = 0;

const server = http.createServer((req, res) => {
    requestCount++;

    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Connection': 'keep-alive',
        'Keep-Alive': 'timeout=5'
    });
    res.end(`Request #${requestCount}`);
});

server.keepAliveTimeout = 5000;

server.listen(PORT, () => {
    console.log(`HTTP Keep-Alive server listening on port ${PORT}`);
    console.log(`Test with: wrk -t4 -c100 -d30s http://127.0.0.1:${PORT}/`);
});
