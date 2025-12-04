import { createHash } from "crypto";

console.log("Testing crypto...");

const hash = createHash('sha256', 'test');
console.log("Hash result:", hash);

console.log("Done!");
