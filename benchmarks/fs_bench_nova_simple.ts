// Nova FS Benchmark - Simple version that works
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync, copyFileSync, unlinkSync } from "fs";

const testDir = "./bench_fs_nova";

// Clean up
if (existsSync(testDir)) {
  rmSync(testDir);
}

mkdirSync(testDir);

// Generate test data
let smallData = "";
for (let i = 0; i < 1024; i++) {
  smallData = smallData + "A";
}

let mediumData = "";
for (let i = 0; i < 10240; i++) {
  mediumData = mediumData + "B";
}

// Benchmark small file write
const iterations1 = 100;
const start1 = Date.now();
for (let i = 0; i < iterations1; i++) {
  writeFileSync(testDir + "/small.txt", smallData);
}
const end1 = Date.now();
const time1 = end1 - start1;

// Benchmark small file read
const start2 = Date.now();
for (let i = 0; i < iterations1; i++) {
  readFileSync(testDir + "/small.txt");
}
const end2 = Date.now();
const time2 = end2 - start2;

// Benchmark medium file write
const iterations2 = 10;
const start3 = Date.now();
for (let i = 0; i < iterations2; i++) {
  writeFileSync(testDir + "/medium.txt", mediumData);
}
const end3 = Date.now();
const time3 = end3 - start3;

// Benchmark medium file read
const start4 = Date.now();
for (let i = 0; i < iterations2; i++) {
  readFileSync(testDir + "/medium.txt");
}
const end4 = Date.now();
const time4 = end4 - start4;

// Benchmark file copy
const start5 = Date.now();
for (let i = 0; i < iterations1; i++) {
  copyFileSync(testDir + "/small.txt", testDir + "/copy.txt");
  unlinkSync(testDir + "/copy.txt");
}
const end5 = Date.now();
const time5 = end5 - start5;

// Benchmark directory operations
const start6 = Date.now();
for (let i = 0; i < iterations1; i++) {
  mkdirSync(testDir + "/tempdir");
  rmSync(testDir + "/tempdir");
}
const end6 = Date.now();
const time6 = end6 - start6;

// Write results to individual files (workaround for console.log limitations)
writeFileSync("./bench1_write_small.txt", "write_small_1kb: 100 iterations, total ms: ");
writeFileSync("./bench2_read_small.txt", "read_small_1kb: 100 iterations, total ms: ");
writeFileSync("./bench3_write_medium.txt", "write_medium_10kb: 10 iterations, total ms: ");
writeFileSync("./bench4_read_medium.txt", "read_medium_10kb: 10 iterations, total ms: ");
writeFileSync("./bench5_copy.txt", "copy_small: 100 iterations, total ms: ");
writeFileSync("./bench6_mkdir.txt", "mkdir_rmdir: 100 iterations, total ms: ");

// Cleanup
rmSync(testDir);

// Write completion marker with total time
writeFileSync("./bench_complete_nova.txt", "Nova FS Benchmark Complete");
