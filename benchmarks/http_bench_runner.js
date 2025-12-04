// HTTP Benchmark Runner
// Measures RPS, latency percentiles (p50, p90, p99), CPU, and Memory usage
const http = require("http");
const { spawn } = require("child_process");

// Configuration
const BENCHMARK_CONFIGS = [
  {
    name: "Hello World",
    servers: [
      {
        name: "Nova",
        command: "./build/Release/nova.exe",
        args: ["benchmarks/http_hello_nova.ts"],
      },
      {
        name: "Node",
        command: "node",
        args: ["benchmarks/http_hello_node.js"],
      },
      {
        name: "Bun",
        command: "bun",
        args: ["benchmarks/http_hello_bun.ts"],
      },
    ],
    url: "http://localhost:3000/",
    method: "GET",
  },
  {
    name: "Routing (GET /users)",
    servers: [
      {
        name: "Nova",
        command: "./build/Release/nova.exe",
        args: ["benchmarks/http_routing_nova.ts"],
      },
      {
        name: "Node",
        command: "node",
        args: ["benchmarks/http_routing_node.js"],
      },
      {
        name: "Bun",
        command: "bun",
        args: ["benchmarks/http_routing_bun.ts"],
      },
    ],
    url: "http://localhost:3000/users",
    method: "GET",
  },
];

const CONCURRENCY_LEVELS = [1, 50, 500];
const DURATION_SECONDS = 10;
const WARMUP_SECONDS = 2;

// Utilities
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function startServer(config) {
  return new Promise((resolve, reject) => {
    const proc = spawn(config.command, config.args, {
      stdio: ["ignore", "pipe", "pipe"],
    });

    let started = false;
    const timeout = setTimeout(() => {
      if (!started) {
        proc.kill();
        reject(new Error(`Server ${config.name} failed to start in time`));
      }
    }, 10000);

    proc.stdout.on("data", (data) => {
      const output = data.toString();
      if (output.includes("running") && !started) {
        started = true;
        clearTimeout(timeout);
        resolve(proc);
      }
    });

    proc.stderr.on("data", (data) => {
      console.error(`[${config.name} stderr]:`, data.toString());
    });

    proc.on("error", (err) => {
      clearTimeout(timeout);
      reject(err);
    });

    proc.on("exit", (code) => {
      if (!started) {
        clearTimeout(timeout);
        reject(new Error(`Server exited with code ${code}`));
      }
    });
  });
}

async function stopServer(proc) {
  return new Promise((resolve) => {
    if (proc.killed) {
      resolve();
      return;
    }

    proc.on("exit", () => {
      resolve();
    });

    proc.kill("SIGTERM");

    // Force kill after 2 seconds
    setTimeout(() => {
      if (!proc.killed) {
        proc.kill("SIGKILL");
      }
    }, 2000);
  });
}

function makeRequest(url, method = "GET") {
  return new Promise((resolve, reject) => {
    const startTime = process.hrtime.bigint();

    const req = http.request(url, { method }, (res) => {
      let data = "";
      res.on("data", (chunk) => {
        data += chunk;
      });
      res.on("end", () => {
        const endTime = process.hrtime.bigint();
        const latency = Number(endTime - startTime) / 1_000_000; // Convert to ms
        resolve({ latency, statusCode: res.statusCode });
      });
    });

    req.on("error", reject);
    req.setTimeout(5000, () => {
      req.destroy();
      reject(new Error("Request timeout"));
    });

    req.end();
  });
}

async function runLoadTest(url, method, concurrency, durationSeconds) {
  const results = [];
  const startTime = Date.now();
  const endTime = startTime + durationSeconds * 1000;

  let activeRequests = 0;
  let completed = false;

  async function worker() {
    while (!completed) {
      try {
        activeRequests++;
        const result = await makeRequest(url, method);
        results.push(result);
        activeRequests--;
      } catch (err) {
        activeRequests--;
        // Skip errors during benchmarking
      }
    }
  }

  // Start workers
  const workers = [];
  for (let i = 0; i < concurrency; i++) {
    workers.push(worker());
  }

  // Wait for duration
  await sleep(durationSeconds * 1000);
  completed = true;

  // Wait for active requests to complete
  while (activeRequests > 0) {
    await sleep(10);
  }

  const actualDuration = (Date.now() - startTime) / 1000;

  // Calculate statistics
  const latencies = results.map((r) => r.latency).sort((a, b) => a - b);
  const successCount = results.filter((r) => r.statusCode === 200).length;

  return {
    totalRequests: results.length,
    successCount,
    rps: results.length / actualDuration,
    avgLatency: latencies.reduce((a, b) => a + b, 0) / latencies.length,
    p50: latencies[Math.floor(latencies.length * 0.5)],
    p90: latencies[Math.floor(latencies.length * 0.9)],
    p99: latencies[Math.floor(latencies.length * 0.99)],
    minLatency: latencies[0],
    maxLatency: latencies[latencies.length - 1],
  };
}

async function getProcessMemory(pid) {
  return new Promise((resolve) => {
    const proc = spawn("powershell", [
      "-Command",
      `(Get-Process -Id ${pid}).WorkingSet64 / 1MB`,
    ]);

    let output = "";
    proc.stdout.on("data", (data) => {
      output += data.toString();
    });

    proc.on("close", () => {
      const memory = parseFloat(output.trim());
      resolve(isNaN(memory) ? 0 : memory);
    });

    proc.on("error", () => resolve(0));
  });
}

async function getProcessCPU(pid) {
  return new Promise((resolve) => {
    const proc = spawn("powershell", [
      "-Command",
      `(Get-Process -Id ${pid}).CPU`,
    ]);

    let output = "";
    proc.stdout.on("data", (data) => {
      output += data.toString();
    });

    proc.on("close", () => {
      const cpu = parseFloat(output.trim());
      resolve(isNaN(cpu) ? 0 : cpu);
    });

    proc.on("error", () => resolve(0));
  });
}

async function monitorResources(proc, durationSeconds) {
  const samples = [];
  const interval = 500; // Sample every 500ms
  const iterations = (durationSeconds * 1000) / interval;

  for (let i = 0; i < iterations; i++) {
    const memory = await getProcessMemory(proc.pid);
    const cpu = await getProcessCPU(proc.pid);
    samples.push({ memory, cpu, timestamp: Date.now() });
    await sleep(interval);
  }

  // Calculate CPU usage per second
  const cpuStart = samples[0].cpu;
  const cpuEnd = samples[samples.length - 1].cpu;
  const cpuPercent =
    ((cpuEnd - cpuStart) / durationSeconds / navigator.hardwareConcurrency) *
    100;

  return {
    avgMemoryMB: samples.reduce((a, b) => a + b.memory, 0) / samples.length,
    maxMemoryMB: Math.max(...samples.map((s) => s.memory)),
    cpuPercent: isNaN(cpuPercent) ? 0 : cpuPercent,
  };
}

async function runBenchmark(benchmarkConfig, serverConfig, concurrency) {
  console.log(
    `\n${"=".repeat(70)}\nBenchmark: ${benchmarkConfig.name} | Runtime: ${
      serverConfig.name
    } | Concurrency: ${concurrency}\n${"=".repeat(70)}`
  );

  // Start server
  console.log(`Starting ${serverConfig.name} server...`);
  let serverProc;
  try {
    serverProc = await startServer(serverConfig);
  } catch (err) {
    console.error(`Failed to start server: ${err.message}`);
    return null;
  }

  // Wait for server to be ready
  await sleep(1000);

  // Warmup
  console.log(`Warming up for ${WARMUP_SECONDS} seconds...`);
  try {
    await runLoadTest(
      benchmarkConfig.url,
      benchmarkConfig.method,
      concurrency,
      WARMUP_SECONDS
    );
  } catch (err) {
    console.error(`Warmup failed: ${err.message}`);
    await stopServer(serverProc);
    return null;
  }

  // Run actual benchmark
  console.log(`Running benchmark for ${DURATION_SECONDS} seconds...`);

  // Start resource monitoring
  const resourcePromise = monitorResources(serverProc, DURATION_SECONDS);

  // Run load test
  let loadTestResult;
  try {
    loadTestResult = await runLoadTest(
      benchmarkConfig.url,
      benchmarkConfig.method,
      concurrency,
      DURATION_SECONDS
    );
  } catch (err) {
    console.error(`Load test failed: ${err.message}`);
    await stopServer(serverProc);
    return null;
  }

  // Get resource usage
  const resources = await resourcePromise;

  // Stop server
  await stopServer(serverProc);
  await sleep(500); // Wait for port to be released

  // Print results
  console.log("\n--- Results ---");
  console.log(`Total Requests: ${loadTestResult.totalRequests}`);
  console.log(`Successful: ${loadTestResult.successCount}`);
  console.log(`RPS: ${loadTestResult.rps.toFixed(2)} req/sec`);
  console.log(`\nLatency:`);
  console.log(`  Min: ${loadTestResult.minLatency.toFixed(2)} ms`);
  console.log(`  Avg: ${loadTestResult.avgLatency.toFixed(2)} ms`);
  console.log(`  p50: ${loadTestResult.p50.toFixed(2)} ms`);
  console.log(`  p90: ${loadTestResult.p90.toFixed(2)} ms`);
  console.log(`  p99: ${loadTestResult.p99.toFixed(2)} ms`);
  console.log(`  Max: ${loadTestResult.maxLatency.toFixed(2)} ms`);
  console.log(`\nResources:`);
  console.log(`  Avg Memory: ${resources.avgMemoryMB.toFixed(2)} MB`);
  console.log(`  Max Memory: ${resources.maxMemoryMB.toFixed(2)} MB`);
  console.log(`  CPU: ${resources.cpuPercent.toFixed(2)}%`);

  return {
    benchmark: benchmarkConfig.name,
    runtime: serverConfig.name,
    concurrency,
    ...loadTestResult,
    ...resources,
  };
}

async function main() {
  console.log("HTTP Performance Benchmark Suite");
  console.log("=================================\n");
  console.log(`Concurrency levels: ${CONCURRENCY_LEVELS.join(", ")}`);
  console.log(`Duration: ${DURATION_SECONDS}s per test`);
  console.log(`Warmup: ${WARMUP_SECONDS}s per test\n`);

  const allResults = [];

  for (const benchmarkConfig of BENCHMARK_CONFIGS) {
    for (const serverConfig of benchmarkConfig.servers) {
      for (const concurrency of CONCURRENCY_LEVELS) {
        const result = await runBenchmark(
          benchmarkConfig,
          serverConfig,
          concurrency
        );
        if (result) {
          allResults.push(result);
        }
      }
    }
  }

  // Print summary
  console.log("\n\n" + "=".repeat(70));
  console.log("SUMMARY");
  console.log("=".repeat(70));

  for (const benchmarkConfig of BENCHMARK_CONFIGS) {
    console.log(`\n${benchmarkConfig.name}:`);
    console.log("-".repeat(70));

    for (const concurrency of CONCURRENCY_LEVELS) {
      console.log(`\nConcurrency: ${concurrency}`);
      console.log(
        "Runtime".padEnd(12) +
          "RPS".padEnd(15) +
          "p50 (ms)".padEnd(12) +
          "p99 (ms)".padEnd(12) +
          "Memory (MB)".padEnd(15) +
          "CPU %"
      );
      console.log("-".repeat(70));

      for (const serverConfig of benchmarkConfig.servers) {
        const result = allResults.find(
          (r) =>
            r.benchmark === benchmarkConfig.name &&
            r.runtime === serverConfig.name &&
            r.concurrency === concurrency
        );

        if (result) {
          console.log(
            result.runtime.padEnd(12) +
              result.rps.toFixed(2).padEnd(15) +
              result.p50.toFixed(2).padEnd(12) +
              result.p99.toFixed(2).padEnd(12) +
              result.avgMemoryMB.toFixed(2).padEnd(15) +
              result.cpuPercent.toFixed(2)
          );
        }
      }
    }
  }
}

main().catch(console.error);
