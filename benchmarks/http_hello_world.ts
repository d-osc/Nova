/**
 * HTTP "Hello World" Benchmark
 *
 * Tests the absolute minimum overhead of HTTP server
 * This is the most common benchmark for HTTP performance
 */

import * as http from 'http';

const PORT = 3000;
const RESPONSE = 'Hello World';

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end(RESPONSE);
});

server.listen(PORT, () => {
    console.log(`HTTP Hello World server listening on port ${PORT}`);
    console.log(`Test with: wrk -t4 -c100 -d30s http://127.0.0.1:${PORT}/`);
});
