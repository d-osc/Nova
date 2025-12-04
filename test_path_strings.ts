// Test Path module to verify strings work
import { dirname } from "path";

console.log("Testing Path module...");
const dir = dirname("/home/user/test.txt");
console.log("dirname result:", dir);

console.log("Path test complete");
