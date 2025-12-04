// Nova FS Benchmark - Working version
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync, statSync, copyFileSync, unlinkSync, readdirSync } from "fs";
import { join } from "path";

const ITERATIONS_SMALL = 100;
const ITERATIONS_MEDIUM = 10;
const SMALL_SIZE = 1024;
const MEDIUM_SIZE = 1024 * 1024;

function generateData(size: number): string {
  let data = "";
  const char = "A";
  for (let i = 0; i < size; i++) {
    data = data + char;
  }
  return data;
}

function benchmark(name: string, fn: any, iterations: number) {
  const start = Date.now();
  for (let i = 0; i < iterations; i++) {
    fn();
  }
  const end = Date.now();
  const totalMs = end - start;
  const avgMs = totalMs / iterations;
  const opsPerSec = 1000 / avgMs;

  // Can't use template literals with numbers, concatenate manually
  // Note: Can't append to file easily, so just write individual result files
  const resultFile = "./bench_result_" + name + ".txt";
  const avgMsStr = avgMs.toFixed(3);
  const result = name + ": " + avgMsStr + "ms avg";
  writeFileSync(resultFile, result);
}

const testDir = "./bench_fs_nova";
if (existsSync(testDir)) {
  rmSync(testDir);
}
mkdirSync(testDir);

const smallData = generateData(SMALL_SIZE);
const mediumData = generateData(MEDIUM_SIZE);

// Start message
writeFileSync("./bench_start.txt", "Nova FS Benchmark Started\n");

// Small file write
benchmark("write_small_1kb", function() {
  writeFileSync(join(testDir, "small.txt"), smallData);
}, ITERATIONS_SMALL);

// Small file read
benchmark("read_small_1kb", function() {
  readFileSync(join(testDir, "small.txt"));
}, ITERATIONS_SMALL);

// Medium file write
benchmark("write_medium_1mb", function() {
  writeFileSync(join(testDir, "medium.txt"), mediumData);
}, ITERATIONS_MEDIUM);

// Medium file read
benchmark("read_medium_1mb", function() {
  readFileSync(join(testDir, "medium.txt"));
}, ITERATIONS_MEDIUM);

// File copy
benchmark("copy_small_file", function() {
  copyFileSync(join(testDir, "small.txt"), join(testDir, "small_copy.txt"));
  unlinkSync(join(testDir, "small_copy.txt"));
}, ITERATIONS_SMALL);

// Directory operations
benchmark("create_delete_dir", function() {
  const dirPath = join(testDir, "tempdir");
  mkdirSync(dirPath);
  rmSync(dirPath);
}, ITERATIONS_SMALL);

// Cleanup
rmSync(testDir);

writeFileSync("./bench_complete.txt", "Benchmark complete!");
