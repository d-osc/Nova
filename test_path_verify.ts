// Test Path module thoroughly
import { dirname, basename, extname } from "path";

const path1 = "/home/user/test.txt";
console.log("Input:", path1);

const dir = dirname(path1);
console.log("dirname:", dir);

const base = basename(path1);
console.log("basename:", base);

const ext = extname(path1);
console.log("extname:", ext);

console.log("All Path tests complete!");
