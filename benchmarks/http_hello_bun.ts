// Bun HTTP Hello World Benchmark
const server = Bun.serve({
  port: 3000,
  fetch(req) {
    return new Response("Hello World", {
      headers: { "Content-Type": "text/plain" },
    });
  },
});

console.log(`Bun server running on http://localhost:${server.port}`);
