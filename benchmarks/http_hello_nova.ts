// Nova HTTP Hello World Benchmark
import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

const PORT = 3000;
server.listen(PORT);
console.log(`Nova server running on http://localhost:${PORT}`);

// Enter event loop to handle requests (run indefinitely with max requests)
server.run(999999);
