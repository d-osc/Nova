// Node.js HTTP Server Benchmark
// Simple server that responds with "Hello World"

const http = require('http');

const server = http.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const port = 3000;
server.listen(port, () => {
  console.log(`Node.js HTTP server listening on port ${port}`);
});
