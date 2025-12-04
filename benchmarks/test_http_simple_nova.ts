import * as http from 'nova:http';

const server = http.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const port = 3000;
server.listen(port);

console.log(`Nova HTTP server listening on port ${port}`);
