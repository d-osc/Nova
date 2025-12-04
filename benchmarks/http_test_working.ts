import { createServer } from "http";

console.log("Creating server...");
const server = createServer((req, res) => {
  console.log("Request received:", req.method, req.url);
  res.writeHead(200);
  res.end("Hello from Nova!");
});

console.log("Starting server on port 3000...");
server.listen(3000);
console.log("Server listening, entering event loop...");

// Handle 10 requests then exit
const count = server.run(10);
console.log(`Handled ${count} requests, exiting.`);
