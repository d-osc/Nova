// Fresh test for isAbsolute
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

// Test 1: Unix absolute
const r1 = isAbsolute("/usr/bin");
if (r1 === 1) {
  writeFileSync("./fresh1.txt", "PASS");
} else {
  writeFileSync("./fresh1.txt", "FAIL");
}

// Test 2: Windows absolute
const r2 = isAbsolute("D:\\Program Files");
if (r2 === 1) {
  writeFileSync("./fresh2.txt", "PASS");
} else {
  writeFileSync("./fresh2.txt", "FAIL");
}

// Test 3: Relative path
const r3 = isAbsolute("src/components");
if (r3 === 0) {
  writeFileSync("./fresh3.txt", "PASS");
} else {
  writeFileSync("./fresh3.txt", "FAIL");
}

// Test 4: Current directory
const r4 = isAbsolute("./index.ts");
if (r4 === 0) {
  writeFileSync("./fresh4.txt", "PASS");
} else {
  writeFileSync("./fresh4.txt", "FAIL");
}

// Test 5: Parent directory
const r5 = isAbsolute("../parent/file.txt");
if (r5 === 0) {
  writeFileSync("./fresh5.txt", "PASS");
} else {
  writeFileSync("./fresh5.txt", "FAIL");
}

writeFileSync("./fresh_done.txt", "Complete");
