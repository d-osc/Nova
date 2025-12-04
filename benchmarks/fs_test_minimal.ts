// Minimal FS test
import * as fs from "fs";
import * as path from "path";

console.log("Testing FS operations...");

const testDir = "./test_fs_min";

// Test 1: mkdir
console.log("1. Creating directory...");
if (fs.existsSync(testDir)) {
  fs.rmSync(testDir, { recursive: true, force: true });
}
fs.mkdirSync(testDir);
console.log("OK");

// Test 2: writeFileSync
console.log("2. Writing file...");
fs.writeFileSync(path.join(testDir, "test.txt"), "Hello World");
console.log("OK");

// Test 3: readFileSync
console.log("3. Reading file...");
const content = fs.readFileSync(path.join(testDir, "test.txt"), "utf8");
console.log(`Read: ${content}`);

// Test 4: statSync
console.log("4. Getting file stats...");
const stat = fs.statSync(path.join(testDir, "test.txt"));
console.log(`Size: ${stat.size} bytes`);

// Test 5: cleanup
console.log("5. Cleaning up...");
fs.rmSync(testDir, { recursive: true, force: true });
console.log("OK");

console.log("\nAll tests passed!");
