import { createServer } from "http";

console.log("Before call");
createServer((req, res) => {
  res.writeHead(200);
  res.end("OK");
});
console.log("After call");
