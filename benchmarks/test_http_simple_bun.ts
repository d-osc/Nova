const port = 3002;

const server = Bun.serve({
  port: port,
  fetch(req) {
    return new Response("Hello World", {
      headers: { "Content-Type": "text/plain" },
    });
  },
});

console.log(`Bun HTTP server listening on port ${port}`);
