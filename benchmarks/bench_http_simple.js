// Simple HTTP Server Benchmark
// Start server and measure response time for 1000 requests

const http = require('http');

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ message: 'Hello, World!', timestamp: Date.now() }));
});

server.listen(0, '127.0.0.1', () => {
    const port = server.address().port;
    const totalRequests = 1000;
    let completed = 0;
    let errors = 0;
    const start = Date.now();

    const makeRequest = () => {
        const req = http.request({
            hostname: '127.0.0.1',
            port: port,
            path: '/',
            method: 'GET'
        }, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                completed++;
                checkDone();
            });
        });
        req.on('error', () => {
            errors++;
            completed++;
            checkDone();
        });
        req.end();
    };

    const checkDone = () => {
        if (completed >= totalRequests) {
            const elapsed = Date.now() - start;
            const rps = Math.round(totalRequests / (elapsed / 1000));
            console.log(`Total requests: ${totalRequests}`);
            console.log(`Errors: ${errors}`);
            console.log(`Time: ${elapsed}ms`);
            console.log(`Requests/sec: ${rps}`);
            server.close();
            process.exit(0);
        }
    };

    // Fire all requests concurrently (max 100 at a time)
    const concurrency = 100;
    let sent = 0;
    const sendBatch = () => {
        while (sent < totalRequests && sent - completed < concurrency) {
            makeRequest();
            sent++;
        }
        if (sent < totalRequests) {
            setTimeout(sendBatch, 1);
        }
    };
    sendBatch();
});
