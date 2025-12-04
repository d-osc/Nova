// Node.js HTTP Hello World Benchmark
const http = require("http");

const server = http.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const PORT = 3000;
server.listen(PORT);
console.log(`Node server running on http://localhost:${PORT}`);
