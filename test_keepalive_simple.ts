import { createServer } from "http";

console.log("Starting server...");

const server = createServer((req, res) => {
  console.log("Request received!");
  res.writeHead(200);
  res.end("Hello World");
});

console.log("Calling listen...");
server.listen(3000);
console.log("Listen completed");

console.log("Calling run...");
server.run(3);
console.log("Run completed");
