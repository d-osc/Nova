import { dirname } from "path";
import { randomUUID } from "crypto";

console.log("Testing Path (working):");
const dir = dirname("/home/user/test.txt");
console.log("  dirname result:", dir);
console.log("  type:", typeof dir);
console.log("");

console.log("Testing Crypto (broken):");
const uuid = randomUUID();
console.log("  randomUUID result:", uuid);
console.log("  type:", typeof uuid);
