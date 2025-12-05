// Final isAbsolute test
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

// Test all cases
const tests = [
  ["/usr/bin", 1],  // Unix absolute
  ["C:\\Windows", 1],  // Windows absolute
  ["D:\\Files", 1],  // Windows absolute with different drive
  ["src/components", 0],  // Relative
  ["./index.ts", 0],  // Current dir relative
  ["../parent", 0]  // Parent dir relative
];

// Run test 1
const r0 = isAbsolute("/usr/bin");
if (r0 === 1) {
  writeFileSync("./final0.txt", "PASS");
} else {
  writeFileSync("./final0.txt", "FAIL");
}

// Run test 2
const r1 = isAbsolute("C:\\Windows");
if (r1 === 1) {
  writeFileSync("./final1.txt", "PASS");
} else {
  writeFileSync("./final1.txt", "FAIL");
}

// Run test 3
const r2 = isAbsolute("D:\\Files");
if (r2 === 1) {
  writeFileSync("./final2.txt", "PASS");
} else {
  writeFileSync("./final2.txt", "FAIL");
}

// Run test 4
const r3 = isAbsolute("src/components");
if (r3 === 0) {
  writeFileSync("./final3.txt", "PASS");
} else {
  writeFileSync("./final3.txt", "FAIL");
}

// Run test 5
const r4 = isAbsolute("./index.ts");
if (r4 === 0) {
  writeFileSync("./final4.txt", "PASS");
} else {
  writeFileSync("./final4.txt", "FAIL");
}

// Run test 6
const r5 = isAbsolute("../parent");
if (r5 === 0) {
  writeFileSync("./final5.txt", "PASS");
} else {
  writeFileSync("./final5.txt", "FAIL");
}

writeFileSync("./final_done.txt", "Complete");
