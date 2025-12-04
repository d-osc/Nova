// Nova HTTP2 Server - Simplified (no console.log)

const server = http2.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

server.listen(3001);
