// Simple FS test without advanced features
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync } from "fs";

const testDir = "./test_fs_working";

// Clean up
if (existsSync(testDir)) {
  rmSync(testDir);
}

// Create directory
mkdirSync(testDir);

// Write and read test
writeFileSync(testDir + "/hello.txt", "Hello Nova!");
const content1 = readFileSync(testDir + "/hello.txt");

writeFileSync(testDir + "/number.txt", "12345");
const content2 = readFileSync(testDir + "/number.txt");

// Write results summary
let summary = "TEST RESULTS:\n";
summary = summary + "1. Directory created: OK\n";
summary = summary + "2. File 1 written: OK\n";
summary = summary + "3. File 1 read: ";
summary = summary + (content1 ? "OK" : "FAIL");
summary = summary + "\n";
summary = summary + "4. File 2 written: OK\n";
summary = summary + "5. File 2 read: ";
summary = summary + (content2 ? "OK" : "FAIL");
summary = summary + "\n";

writeFileSync(testDir + "/results.txt", summary);

// Cleanup
rmSync(testDir);
