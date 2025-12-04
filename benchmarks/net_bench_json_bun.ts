// Bun HTTP Server - JSON Response Benchmark

const port = 3002;

const server = Bun.serve({
  port: port,
  fetch(req) {
    const data = {
      message: "Hello World",
      timestamp: Date.now(),
      status: "ok",
      data: {
        user: "test",
        items: [1, 2, 3, 4, 5]
      }
    };

    return new Response(JSON.stringify(data), {
      headers: { "Content-Type": "application/json" },
    });
  },
});

console.log(`Bun HTTP JSON server listening on port ${port}`);
