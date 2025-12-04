// Comprehensive verification of server.run() implementation

const http = require("http");

console.log("Creating HTTP server...");
const server = http.createServer((req: any, res: any) => {
    console.log("Handling request");
    res.end("Test response");
});

console.log("Server created successfully");

console.log("Starting server on port 8081...");
server.listen(8081, "127.0.0.1");

console.log("Calling server.run(1) - should handle 1 request then exit");
const result = server.run(1);

console.log("server.run() completed");
console.log("Result value (should be integer): " + result);
