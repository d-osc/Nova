// Test server.run() method - run forever

const http = require("http");

const server = http.createServer((req: any, res: any) => {
    console.log("Request received: " + req.url);
    res.end("Hello World!");
});

server.listen(8080, "127.0.0.1");
console.log("Server listening on port 8080");
console.log("Press Ctrl+C to stop");

// Run forever (maxRequests = 0)
server.run();
