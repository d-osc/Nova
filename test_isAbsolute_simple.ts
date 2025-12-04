// Simple isAbsolute test
import { isAbsolute } from "path";
import { writeFileSync } from "fs";

const result1 = isAbsolute("/home/user");
const result2 = isAbsolute("C:\\Windows\\System32");
const result3 = isAbsolute("relative/path");
const result4 = isAbsolute("./current/path");

writeFileSync("./abs_test1.txt", "Unix /home/user");
writeFileSync("./abs_test2.txt", "Windows C:\\Windows");
writeFileSync("./abs_test3.txt", "relative/path");
writeFileSync("./abs_test4.txt", "./current/path");

writeFileSync("./complete_isabs.txt", "Test complete");
