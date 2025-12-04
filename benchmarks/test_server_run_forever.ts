import { createServer } from "http";

console.log("Starting server...");

const server = createServer((req, res) => {
  console.log("REQUEST RECEIVED!");
  console.log("Method:", req.method);
  console.log("URL:", req.url);
  res.writeHead(200);
  res.end("Hello from Nova!");
});

server.listen(3000);
console.log("Server listening on port 3000");
console.log("Entering event loop...");

// Run forever (0 = no limit)
server.run(0);

console.log("Event loop exited");
