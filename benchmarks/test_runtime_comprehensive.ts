import { createServer } from "http";

console.log("=== STARTING TEST ===");

console.log("Step 1: About to create server");
const server = createServer((req, res) => {
  console.log("CALLBACK CALLED!");
  res.writeHead(200);
  res.end("OK");
});
console.log("Step 2: Server created");

console.log("Step 3: About to call listen");
server.listen(3000);
console.log("Step 4: Listen called");

console.log("Step 5: About to call run");
const result = server.run(1);
console.log("Step 6: Run returned");

console.log("=== TEST COMPLETE ===");
console.log("Result:", result);
