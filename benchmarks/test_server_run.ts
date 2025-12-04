import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("OK");
});

server.listen(3000);
console.log("Server listening on port 3000");
console.log("Calling server.run(5)...");
const count = server.run(5);
console.log("server.run() returned:", count);
