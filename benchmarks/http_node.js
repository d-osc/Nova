// Node.js HTTP Hello World Benchmark Server
const http = require('http');

const port = process.env.PORT || 3000;
const host = '0.0.0.0';

const server = http.createServer((req, res) => {
    res.statusCode = 200;
    res.setHeader('Content-Type', 'text/plain');
    res.end('Hello World!');
});

server.listen(port, host, () => {
    console.log(`Node.js HTTP server listening on http://${host}:${port}`);
});
