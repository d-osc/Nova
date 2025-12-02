// HTTP Server Benchmark (Node.js compatible)
const http = require('http');

const server = http.createServer((req, res) => {
    if (req.url === '/json') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ message: 'Hello, World!', timestamp: Date.now() }));
    } else if (req.url === '/text') {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Hello, World!');
    } else {
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.end('<h1>Hello, World!</h1>');
    }
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
