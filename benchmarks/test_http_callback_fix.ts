import { createServer } from "http";

// Test 1: Simple arrow function callback
const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("Hello from arrow function!");
});

server.listen(3000);
console.log("Server listening on port 3000");
server.run(1);  // Handle 1 request then exit
