import { createServer } from "http";

console.log("TEST 1: Create server");
const server = createServer((req, res) => {
  res.writeHead(200);
  res.end("OK");
});

console.log("TEST 2: Listen on port 3000");
server.listen(3000);

console.log("TEST 3: Call server.run(3)");
const count = server.run(3);

console.log("TEST 4: Returned from server.run()");
console.log("TEST 5: count value type:", typeof count);
console.log("TEST 6: count value:", count);
console.log("TEST 7: count === undefined?", count === undefined);
console.log("TEST 8: count === null?", count === null);
console.log("TEST 9: count === 0?", count === 0);
