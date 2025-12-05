// Debug test - write actual return values
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

const test1 = isAbsolute("/usr/bin");
const test2 = isAbsolute("C:\\Windows");
const test3 = isAbsolute("src/components");

// Write tests with different branches
if (test1 === 0) {
  writeFileSync("./dbg2_test1.txt", "returned_0");
} else if (test1 === 1) {
  writeFileSync("./dbg2_test1.txt", "returned_1");
} else {
  writeFileSync("./dbg2_test1.txt", "returned_other");
}

if (test2 === 0) {
  writeFileSync("./dbg2_test2.txt", "returned_0");
} else if (test2 === 1) {
  writeFileSync("./dbg2_test2.txt", "returned_1");
} else {
  writeFileSync("./dbg2_test2.txt", "returned_other");
}

if (test3 === 0) {
  writeFileSync("./dbg2_test3.txt", "returned_0");
} else if (test3 === 1) {
  writeFileSync("./dbg2_test3.txt", "returned_1");
} else {
  writeFileSync("./dbg2_test3.txt", "returned_other");
}

writeFileSync("./dbg2_done.txt", "Done");
