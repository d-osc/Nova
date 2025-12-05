// Test that server.run() compiles correctly

const http = require("http");

const server = http.createServer((req: any, res: any) => {
    res.end("OK");
});

server.listen(8080);

// Test both variants:
// 1. Run with maxRequests
const result1 = server.run(5);

// 2. Run forever (would be commented out in real use)
// server.run();
