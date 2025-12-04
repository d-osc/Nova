import { createServer } from "http";

const server = createServer((req, res) => {
  console.log("✓ Request received!");
  res.writeHead(200);
  res.end("Hello from Nova HTTP!");
});

console.log("Starting HTTP server on port 3000...");
server.listen(3000);

const count = server.run(3);
console.log(`Server handled ${count} requests successfully!`);
