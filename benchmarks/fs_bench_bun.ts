// Bun File System Benchmark
import * as fs from "fs";
import * as path from "path";

const ITERATIONS = 100;
const WARMUP = 10;

// Test data sizes
const SMALL_SIZE = 1024;        // 1 KB
const MEDIUM_SIZE = 1024 * 1024; // 1 MB
const LARGE_SIZE = 10 * 1024 * 1024; // 10 MB

// Benchmark results
const results: any = {
  runtime: "Bun",
  tests: {}
};

// Generate test data
function generateData(size: number): string {
  const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  let data = "";
  for (let i = 0; i < size; i++) {
    data += chars[i % chars.length];
  }
  return data;
}

// Benchmark helper
function benchmark(name: string, fn: () => void, iterations: number = ITERATIONS): number {
  // Warmup
  for (let i = 0; i < WARMUP; i++) {
    fn();
  }

  // Measure
  const start = Date.now();
  for (let i = 0; i < iterations; i++) {
    fn();
  }
  const end = Date.now();

  const totalMs = end - start;
  const avgMs = totalMs / iterations;
  const opsPerSec = 1000 / avgMs;

  results.tests[name] = {
    totalMs,
    avgMs,
    opsPerSec,
    iterations
  };

  console.log(`${name}: ${avgMs.toFixed(3)}ms avg, ${opsPerSec.toFixed(0)} ops/sec`);
  return avgMs;
}

console.log("Bun File System Benchmark");
console.log("==========================\n");

// Setup test directory
const testDir = "./bench_fs_test";
if (fs.existsSync(testDir)) {
  fs.rmSync(testDir, { recursive: true });
}
fs.mkdirSync(testDir);

// Test data
const smallData = generateData(SMALL_SIZE);
const mediumData = generateData(MEDIUM_SIZE);
const largeData = generateData(LARGE_SIZE);

console.log("1. Small File Operations (1 KB)");
console.log("--------------------------------");

benchmark("write_small_file", () => {
  fs.writeFileSync(path.join(testDir, "small.txt"), smallData);
});

benchmark("read_small_file", () => {
  fs.readFileSync(path.join(testDir, "small.txt"), "utf8");
});

benchmark("stat_small_file", () => {
  fs.statSync(path.join(testDir, "small.txt"));
});

console.log("\n2. Medium File Operations (1 MB)");
console.log("----------------------------------");

benchmark("write_medium_file", () => {
  fs.writeFileSync(path.join(testDir, "medium.txt"), mediumData);
}, 10); // Fewer iterations for larger files

benchmark("read_medium_file", () => {
  fs.readFileSync(path.join(testDir, "medium.txt"), "utf8");
}, 10);

console.log("\n3. Large File Operations (10 MB)");
console.log("----------------------------------");

benchmark("write_large_file", () => {
  fs.writeFileSync(path.join(testDir, "large.txt"), largeData);
}, 5);

benchmark("read_large_file", () => {
  fs.readFileSync(path.join(testDir, "large.txt"), "utf8");
}, 5);

console.log("\n4. Directory Operations");
console.log("-----------------------");

benchmark("create_directory", () => {
  const dirPath = path.join(testDir, `dir_${Date.now()}`);
  fs.mkdirSync(dirPath);
  fs.rmdirSync(dirPath);
});

// Create test files for readdir
for (let i = 0; i < 100; i++) {
  fs.writeFileSync(path.join(testDir, `file_${i}.txt`), "test");
}

benchmark("list_directory", () => {
  fs.readdirSync(testDir);
});

console.log("\n5. File Copy Operations");
console.log("------------------------");

benchmark("copy_small_file", () => {
  fs.copyFileSync(
    path.join(testDir, "small.txt"),
    path.join(testDir, "small_copy.txt")
  );
  fs.unlinkSync(path.join(testDir, "small_copy.txt"));
});

benchmark("copy_medium_file", () => {
  fs.copyFileSync(
    path.join(testDir, "medium.txt"),
    path.join(testDir, "medium_copy.txt")
  );
  fs.unlinkSync(path.join(testDir, "medium_copy.txt"));
}, 10);

console.log("\n6. JSON Operations");
console.log("-------------------");

const jsonData = {
  name: "test",
  value: 123,
  nested: {
    array: [1, 2, 3, 4, 5],
    object: { a: 1, b: 2, c: 3 }
  }
};

benchmark("write_json", () => {
  fs.writeFileSync(
    path.join(testDir, "data.json"),
    JSON.stringify(jsonData)
  );
});

benchmark("read_and_parse_json", () => {
  const content = fs.readFileSync(path.join(testDir, "data.json"), "utf8");
  JSON.parse(content);
});

console.log("\n7. File Exists & Stats");
console.log("-----------------------");

benchmark("file_exists", () => {
  fs.existsSync(path.join(testDir, "small.txt"));
});

benchmark("file_stat", () => {
  const stat = fs.statSync(path.join(testDir, "small.txt"));
  stat.size;
  stat.isFile();
});

console.log("\n8. Delete Operations");
console.log("---------------------");

// Create files to delete
for (let i = 0; i < 10; i++) {
  fs.writeFileSync(path.join(testDir, `delete_${i}.txt`), "test");
}

benchmark("delete_file", () => {
  const filePath = path.join(testDir, `delete_${Date.now()}.txt`);
  fs.writeFileSync(filePath, "test");
  fs.unlinkSync(filePath);
}, 50);

// Cleanup
console.log("\nCleaning up...");
fs.rmSync(testDir, { recursive: true });

console.log("\nBenchmark Complete!");
console.log("\nResults Summary:");
console.log(JSON.stringify(results, null, 2));
