// Test server.run() method

const http = require("http");

const server = http.createServer((req: any, res: any) => {
    console.log("Request received: " + req.url);
    res.end("Hello from server.run()!");
});

server.listen(8080, "127.0.0.1");
console.log("Server listening on port 8080");

// Test 1: Run with limit (handle 3 requests then exit)
const result = server.run(3);
console.log("Handled " + result + " requests");
