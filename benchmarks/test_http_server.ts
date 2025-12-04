import { createServer } from "http";

const server = createServer((req, res) => {
  console.log("Request received!");
  res.writeHead(200);
  res.end("Hello from Nova!");
});

console.log("Server starting on port 3000...");
server.listen(3000);
console.log("Server listening, running 5 requests...");

const count = server.run(5);
console.log(`Server handled ${count} requests`);
