// Bun HTTP Hello World Benchmark Server
const port = parseInt(process.env.PORT || "3000");

Bun.serve({
    port,
    fetch(req) {
        return new Response("Hello World!", {
            headers: { "Content-Type": "text/plain" },
        });
    },
});

console.log(`Bun HTTP server listening on http://0.0.0.0:${port}`);
