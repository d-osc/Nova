// Node.js HTTP Server - JSON Response Benchmark

const http = require('http');

const server = http.createServer((req, res) => {
  const data = {
    message: "Hello World",
    timestamp: Date.now(),
    status: "ok",
    data: {
      user: "test",
      items: [1, 2, 3, 4, 5]
    }
  };

  const json = JSON.stringify(data);
  res.writeHead(200, {
    "Content-Type": "application/json",
    "Content-Length": json.length
  });
  res.end(json);
});

const port = 3002;
server.listen(port, () => {
  console.log(`Node.js HTTP JSON server listening on port ${port}`);
});
