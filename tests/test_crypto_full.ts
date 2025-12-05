import { createHash, createHmac, randomBytesHex, randomUUID } from "crypto";

console.log("=== Testing Nova Crypto Module ===");
console.log("");

// Test 1: SHA256
console.log("Test 1: SHA256 Hash");
const hash1 = createHash('sha256', 'test');
console.log("Input: 'test'");
console.log("SHA256:", hash1);
console.log("");

// Test 2: MD5
console.log("Test 2: MD5 Hash");
const hash2 = createHash('md5', 'hello world');
console.log("Input: 'hello world'");
console.log("MD5:", hash2);
console.log("");

// Test 3: HMAC
console.log("Test 3: HMAC-SHA256");
const hmac = createHmac('sha256', 'secret-key', 'message');
console.log("Key: 'secret-key', Message: 'message'");
console.log("HMAC:", hmac);
console.log("");

// Test 4: Random Bytes (Hex)
console.log("Test 4: Random Bytes (16 bytes as hex)");
const rand1 = randomBytesHex(16);
console.log("Hex (32 chars):", rand1);
console.log("");

// Test 5: Random UUID
console.log("Test 5: Random UUID");
const uuid1 = randomUUID();
const uuid2 = randomUUID();
const uuid3 = randomUUID();
console.log("UUID 1:", uuid1);
console.log("UUID 2:", uuid2);
console.log("UUID 3:", uuid3);
console.log("");

console.log("=== All Tests Complete ===");
