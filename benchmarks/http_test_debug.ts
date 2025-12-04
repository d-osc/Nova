import { createServer } from "http";

console.log("Test 1: Creating server");
const server = createServer((request, response) => {
  console.log("Inside callback");
  console.log("About to call writeHead");
  response.writeHead(200);
  console.log("Called writeHead");
  console.log("About to call end");
  response.end("Hi");
  console.log("Called end");
});

console.log("Test 2: Server created");
