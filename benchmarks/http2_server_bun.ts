// Bun HTTP/2 Server Benchmark
// Note: Bun doesn't have native http2 module yet, using HTTP/1.1 with h2c upgrade
const server = Bun.serve({
  port: 3001,
  fetch(req) {
    return new Response("Hello HTTP/2 from Bun!");
  },
});

console.log(`Bun server (HTTP/1.1) listening on port ${server.port}`);
