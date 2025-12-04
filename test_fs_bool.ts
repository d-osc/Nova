// Test FS boolean return values
import { statSync } from "fs";
import { writeFileSync } from "fs";

// Create a test file first
writeFileSync("./test_for_stat.txt", "test");

// Get stats
const stats = statSync("./test_for_stat.txt");

// Test isFile
const isF = stats.isFile();

if (isF === 0) {
  writeFileSync("./fs_bool_test1.txt", "isFile_returned_0");
} else if (isF === 1) {
  writeFileSync("./fs_bool_test1.txt", "isFile_returned_1");
} else {
  writeFileSync("./fs_bool_test1.txt", "isFile_returned_other");
}

// Test isDirectory
const isD = stats.isDirectory();

if (isD === 0) {
  writeFileSync("./fs_bool_test2.txt", "isDirectory_returned_0");
} else if (isD === 1) {
  writeFileSync("./fs_bool_test2.txt", "isDirectory_returned_1");
} else {
  writeFileSync("./fs_bool_test2.txt", "isDirectory_returned_other");
}

writeFileSync("./fs_bool_done.txt", "Done");
