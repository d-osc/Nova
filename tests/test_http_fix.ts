import http from "nova:http";

console.log("Starting HTTP server test...");

const server = http.createServer((req, res) => {
    console.log("Request received!");
    console.log("Method:", req.method);
    console.log("URL:", req.url);

    res.statusCode = 200;
    res.setHeader("Content-Type", "text/plain");
    res.end("Hello from Nova HTTP server!");
});

server.listen(3000, "127.0.0.1", () => {
    console.log("Server listening on http://127.0.0.1:3000");
    console.log("You can now test with: curl http://127.0.0.1:3000");
});

// Run the server event loop to handle ONE request
console.log("Calling server.run() to handle requests...");
const result = server.run(1);  // Handle 1 request
console.log("server.run() returned:", result);

server.close();
console.log("Server closed");
