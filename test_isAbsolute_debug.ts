// Debug isAbsolute test
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

// Test Unix absolute path
const test1 = "/home/user";
const result1 = isAbsolute(test1);

// Test Windows absolute path
const test2 = "C:\\Windows\\System32";
const result2 = isAbsolute(test2);

// Test relative path
const test3 = "relative/path";
const result3 = isAbsolute(test3);

// Write results - write different text based on result
if (result1 === 0) {
  writeFileSync("./debug_test1.txt", "FAIL: /home/user returned 0");
} else {
  writeFileSync("./debug_test1.txt", "SUCCESS: /home/user returned 1");
}

if (result2 === 0) {
  writeFileSync("./debug_test2.txt", "FAIL: C:\\Windows returned 0");
} else {
  writeFileSync("./debug_test2.txt", "SUCCESS: C:\\Windows returned 1");
}

if (result3 === 0) {
  writeFileSync("./debug_test3.txt", "SUCCESS: relative/path returned 0");
} else {
  writeFileSync("./debug_test3.txt", "FAIL: relative/path returned 1");
}

writeFileSync("./debug_complete.txt", "Debug test complete");
