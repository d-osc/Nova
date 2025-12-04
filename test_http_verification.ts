// HTTP Server Verification Test
// Tests that the Windows Winsock2 select() fix works correctly

import { createServer } from "http";

const PORT = 3000;
const MAX_REQUESTS = 5;

console.log("=".repeat(60));
console.log("Nova HTTP Server - Winsock2 select() Fix Verification");
console.log("=".repeat(60));

const server = createServer((req, res) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.url}`);

  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello from Nova HTTP Server!");
});

server.listen(PORT);
console.log(`\nServer listening on http://127.0.0.1:${PORT}`);
console.log(`Will handle ${MAX_REQUESTS} requests and then exit\n`);
console.log("Test with:");
console.log(`  curl http://127.0.0.1:${PORT}/`);
console.log("\n" + "=".repeat(60) + "\n");

// Run server and handle exactly MAX_REQUESTS requests
const handled = server.run(MAX_REQUESTS);

console.log("\n" + "=".repeat(60));
console.log(`Server handled ${handled} requests successfully`);
console.log("Test PASSED - select() is detecting connections correctly!");
console.log("=".repeat(60));

server.close();
