// Nova Path Module Benchmark
import { dirname, basename, extname, normalize, resolve, isAbsolute, relative } from "path";
import { writeFileSync } from "fs";

const iterations = 10000;

// Test paths (5 paths to test with)
const path1 = "/home/user/documents/file.txt";
const path2 = "C:\\Users\\Admin\\Desktop\\project\\src\\index.js";
const path3 = "../relative/path/to/file.ts";
const path4 = "./current/directory/file.json";
const path5 = "/var/www/html/index.html";

// Benchmark dirname
const start1 = Date.now();
for (let i = 0; i < iterations; i++) {
  dirname(path1);
  dirname(path2);
  dirname(path3);
  dirname(path4);
  dirname(path5);
}
const end1 = Date.now();
const dirnameTime = end1 - start1;

// Benchmark basename
const start2 = Date.now();
for (let i = 0; i < iterations; i++) {
  basename(path1);
  basename(path2);
  basename(path3);
  basename(path4);
  basename(path5);
}
const end2 = Date.now();
const basenameTime = end2 - start2;

// Benchmark extname
const start3 = Date.now();
for (let i = 0; i < iterations; i++) {
  extname(path1);
  extname(path2);
  extname(path3);
  extname(path4);
  extname(path5);
}
const end3 = Date.now();
const extnameTime = end3 - start3;

// Benchmark normalize
const start4 = Date.now();
for (let i = 0; i < iterations; i++) {
  normalize(path1);
  normalize(path2);
  normalize(path3);
  normalize(path4);
  normalize(path5);
}
const end4 = Date.now();
const normalizeTime = end4 - start4;

// Benchmark resolve
const start5 = Date.now();
for (let i = 0; i < iterations; i++) {
  resolve(path1);
  resolve(path2);
  resolve(path3);
  resolve(path4);
  resolve(path5);
}
const end5 = Date.now();
const resolveTime = end5 - start5;

// Benchmark isAbsolute
const start6 = Date.now();
for (let i = 0; i < iterations; i++) {
  isAbsolute(path1);
  isAbsolute(path2);
  isAbsolute(path3);
  isAbsolute(path4);
  isAbsolute(path5);
}
const end6 = Date.now();
const isAbsoluteTime = end6 - start6;

// Benchmark relative
const start7 = Date.now();
for (let i = 0; i < iterations; i++) {
  relative("/home/user", "/home/user/documents/file.txt");
}
const end7 = Date.now();
const relativeTime = end7 - start7;

// Write results to files (workaround for console.log limitations)
writeFileSync("./path_bench_dirname.txt", "dirname");
writeFileSync("./path_bench_basename.txt", "basename");
writeFileSync("./path_bench_extname.txt", "extname");
writeFileSync("./path_bench_normalize.txt", "normalize");
writeFileSync("./path_bench_resolve.txt", "resolve");
writeFileSync("./path_bench_isAbsolute.txt", "isAbsolute");
writeFileSync("./path_bench_relative.txt", "relative");

// Write completion marker
writeFileSync("./path_bench_complete_nova.txt", "Nova Path Benchmark Complete");
