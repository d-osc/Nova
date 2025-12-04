import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("Hello World!");
});

server.listen(3000);
console.log("Nova server ready on port 3000");

// Run forever (until killed)
server.run(999999999);
