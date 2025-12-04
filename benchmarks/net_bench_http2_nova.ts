// Nova HTTP2 Server Benchmark

const server = http2.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const port = 3001;
server.listen(port);

console.log(`Nova HTTP2 server listening on port ${port}`);
