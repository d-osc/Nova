import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("OK");
});

server.listen(3000);
const result = server.run(5);
