const port = process.env.PORT || 3002;

Bun.serve({
    port: port,
    fetch(req) {
        return new Response('Hello World');
    }
});

console.log('Server running on port ' + port);
