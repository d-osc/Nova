import { createServer } from "http";

console.log("Starting server...");

const server = createServer((req, res) => {
  console.log("Request received");
  res.writeHead(200);
  res.end("Hello");
});

console.log("Server created");
server.listen(3000);
console.log("Server listening on port 3000");
