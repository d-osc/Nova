// Test HTTP server with select() debugging on Windows
const http = require('nova:http');

const server = http.createServer((req: any, res: any) => {
    console.log(`Received ${req.method} ${req.url}`);
    res.statusCode = 200;
    res.setHeader('Content-Type', 'text/plain');
    res.end('Hello from Nova HTTP Server!\n');
});

server.listen(3000, '127.0.0.1', () => {
    console.log('Server listening on http://127.0.0.1:3000');
    console.log('Try: curl http://127.0.0.1:3000');
});

// Run for a limited time (handle 3 requests then exit)
const maxRequests = 3;
const result = server.run(maxRequests);
console.log(`Server handled ${result} requests`);
