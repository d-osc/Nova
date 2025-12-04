// Test FS with destructuring import (like http module)
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync, statSync } from "fs";

console.log("Testing FS with destructuring import...");

const testDir = "./test_fs_simple3";

if (existsSync(testDir)) {
  rmSync(testDir);
}
mkdirSync(testDir);
console.log("1. Directory created ✓");

// Write file
writeFileSync(testDir + "/test.txt", "Hello Nova World!");
console.log("2. File written ✓");

// Read file
const content = readFileSync(testDir + "/test.txt");
console.log("3. File read:");
console.log("   Type:", typeof content);
console.log("   Content:", content);

// Get stats
const stat = statSync(testDir + "/test.txt");
console.log("4. File stats:");
console.log("   Size:", stat.size);
console.log("   IsFile:", stat.isFile());

// Cleanup
rmSync(testDir);
console.log("5. Cleaned up ✓");

console.log("\nAll tests passed!");
