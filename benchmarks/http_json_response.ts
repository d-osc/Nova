/**
 * HTTP JSON Response Benchmark
 *
 * Tests JSON serialization and response performance
 */

import * as http from 'http';

const PORT = 3001;

const server = http.createServer((req, res) => {
    const data = {
        message: 'Hello World',
        timestamp: Date.now(),
        status: 'success',
        data: {
            items: [1, 2, 3, 4, 5],
            count: 5
        }
    };

    const json = JSON.stringify(data);

    res.writeHead(200, {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(json)
    });
    res.end(json);
});

server.listen(PORT, () => {
    console.log(`HTTP JSON server listening on port ${PORT}`);
    console.log(`Test with: wrk -t4 -c100 -d30s http://127.0.0.1:${PORT}/`);
});
