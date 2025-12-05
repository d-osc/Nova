import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("Hello World");
});

server.listen(3000);
server.run(5);
