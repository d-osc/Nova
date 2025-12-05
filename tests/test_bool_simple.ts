// Test boolean without strict equality
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

const test1 = isAbsolute("/usr/bin");
const test2 = isAbsolute("src/components");

// Test with if statement (truthy/falsy)
if (test1) {
  writeFileSync("./bool_simple1.txt", "test1_truthy");
} else {
  writeFileSync("./bool_simple1.txt", "test1_falsy");
}

if (test2) {
  writeFileSync("./bool_simple2.txt", "test2_truthy");
} else {
  writeFileSync("./bool_simple2.txt", "test2_falsy");
}

writeFileSync("./bool_simple_done.txt", "Done");
