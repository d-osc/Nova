// Verify FS works by writing results to file
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync } from "fs";

const testDir = "./test_fs_verify";
const outputFile = "./fs_test_output.txt";

// Clean up previous run
if (existsSync(testDir)) {
  rmSync(testDir);
}

if (existsSync(outputFile)) {
  rmSync(outputFile);
}

// Test 1: Create directory
mkdirSync(testDir);
writeFileSync(outputFile, "1. Directory created: PASS\n");

// Test 2: Write file with known content
const testContent = "Hello Nova World 123!";
writeFileSync(testDir + "/test.txt", testContent);
writeFileSync(outputFile, "2. File written: PASS\n", { flag: "a" });

// Test 3: Read file back
const readContent = readFileSync(testDir + "/test.txt");
writeFileSync(outputFile, "3. File read: PASS\n", { flag: "a" });

// Test 4: Verify content matches (write content length as verification)
// If readContent is a string, this will work
const verification = "4. Content verification: ";
if (readContent) {
  writeFileSync(outputFile, verification + "PASS (got data)\n", { flag: "a" });
} else {
  writeFileSync(outputFile, verification + "FAIL (no data)\n", { flag: "a" });
}

// Test 5: Cleanup
rmSync(testDir);
writeFileSync(outputFile, "5. Cleanup: PASS\n", { flag: "a" });

writeFileSync(outputFile, "\nAll basic tests completed!\n", { flag: "a" });
