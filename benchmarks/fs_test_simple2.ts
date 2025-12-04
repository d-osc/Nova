// Test FS without encoding parameter
import * as fs from "fs";

console.log("Testing FS - no encoding...");

const testDir = "./test_fs_simple2";

if (fs.existsSync(testDir)) {
  fs.rmSync(testDir, { recursive: true, force: true });
}
fs.mkdirSync(testDir);

// Write file
fs.writeFileSync(testDir + "/test.txt", "Hello World 123");

// Read WITHOUT encoding
const content = fs.readFileSync(testDir + "/test.txt");
console.log("Read content type:", typeof content);
console.log("Read content:", content);

// Cleanup
fs.rmSync(testDir, { recursive: true, force: true });

console.log("Done!");
