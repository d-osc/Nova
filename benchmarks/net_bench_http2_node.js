// Node.js HTTP2 Server Benchmark

const http2 = require('http2');

const server = http2.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const port = 3001;
server.listen(port, () => {
  console.log(`Node.js HTTP2 server listening on port ${port}`);
});
