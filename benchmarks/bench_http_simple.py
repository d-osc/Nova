#!/usr/bin/env python3
"""
Simple HTTP Benchmark - Nova vs Node vs Bun
Measures throughput, latency, and resource usage
"""

import subprocess
import time
import psutil
import requests
from statistics import mean, median
import sys

PORT = 3000
URL = f"http://127.0.0.1:{PORT}/"

def test_server(name, command, num_requests=100, warmup=10):
    """Test a single server implementation"""
    print(f"\n{'='*60}")
    print(f"Testing {name}")
    print(f"{'='*60}")

    # Start server
    print(f"Starting {name} server...")
    try:
        process = subprocess.Popen(
            command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
    except Exception as e:
        print(f"ERROR: Failed to start server: {e}")
        return None

    # Wait for server to start
    print("Waiting for server to start...", end="", flush=True)
    for i in range(30):  # Wait up to 30 seconds
        try:
            response = requests.get(URL, timeout=1)
            if response.status_code == 200:
                print(" Ready!")
                break
        except:
            print(".", end="", flush=True)
            time.sleep(1)
    else:
        print("\nERROR: Server failed to start within 30 seconds")
        process.kill()
        return None

    # Get process for monitoring
    try:
        proc = psutil.Process(process.pid)
        children = proc.children(recursive=True)
        if children:
            proc = children[0]  # Monitor child process if exists
    except:
        print("WARNING: Could not attach to process for monitoring")
        proc = None

    # Warmup
    print(f"Warming up with {warmup} requests...")
    for _ in range(warmup):
        try:
            requests.get(URL, timeout=5)
        except:
            pass

    time.sleep(0.5)

    # Run benchmark
    print(f"Running benchmark with {num_requests} requests...")
    latencies = []
    successes = 0
    failures = 0
    memory_samples = []
    cpu_samples = []

    start_time = time.time()

    for i in range(num_requests):
        req_start = time.time()
        try:
            response = requests.get(URL, timeout=5)
            latency = (time.time() - req_start) * 1000  # Convert to ms

            if response.status_code == 200:
                successes += 1
                latencies.append(latency)
            else:
                failures += 1
        except Exception as e:
            failures += 1

        # Sample resource usage every 10 requests
        if proc and (i + 1) % 10 == 0:
            try:
                mem_info = proc.memory_info()
                memory_samples.append(mem_info.rss / 1024 / 1024)  # MB
                cpu_samples.append(proc.cpu_percent(interval=0.01))
            except:
                pass

        # Progress
        if (i + 1) % 10 == 0:
            print(f"  Progress: {i+1}/{num_requests} requests", end="\r")

    end_time = time.time()
    duration = end_time - start_time

    print(f"  Progress: {num_requests}/{num_requests} requests - Done!   ")

    # Final resource usage
    if proc:
        try:
            mem_info = proc.memory_info()
            final_memory = mem_info.rss / 1024 / 1024  # MB
        except:
            final_memory = None
    else:
        final_memory = None

    # Cleanup
    print("Stopping server...")
    try:
        process.terminate()
        process.wait(timeout=5)
    except:
        process.kill()

    # Calculate metrics
    if successes > 0:
        rps = successes / duration
        avg_latency = mean(latencies)
        median_latency = median(latencies)
        p95_latency = sorted(latencies)[int(len(latencies) * 0.95)] if len(latencies) > 20 else avg_latency
        p99_latency = sorted(latencies)[int(len(latencies) * 0.99)] if len(latencies) > 50 else avg_latency
    else:
        rps = avg_latency = median_latency = p95_latency = p99_latency = 0

    avg_memory = mean(memory_samples) if memory_samples else (final_memory or 0)
    avg_cpu = mean(cpu_samples) if cpu_samples else 0

    results = {
        'name': name,
        'successes': successes,
        'failures': failures,
        'duration': duration,
        'rps': rps,
        'avg_latency': avg_latency,
        'median_latency': median_latency,
        'p95_latency': p95_latency,
        'p99_latency': p99_latency,
        'avg_memory_mb': avg_memory,
        'final_memory_mb': final_memory or 0,
        'avg_cpu_percent': avg_cpu
    }

    # Print results
    print(f"\nResults for {name}:")
    print(f"  Requests: {successes} successful, {failures} failed")
    print(f"  Duration: {duration:.2f} seconds")
    print(f"  Throughput: {rps:.2f} req/sec")
    print(f"  Latency:")
    print(f"    Average: {avg_latency:.2f} ms")
    print(f"    Median:  {median_latency:.2f} ms")
    print(f"    P95:     {p95_latency:.2f} ms")
    print(f"    P99:     {p99_latency:.2f} ms")
    print(f"  Memory: {avg_memory:.2f} MB avg, {final_memory or 0:.2f} MB final")
    if cpu_samples:
        print(f"  CPU: {avg_cpu:.1f}% average")

    return results

def main():
    num_requests = 200 if len(sys.argv) < 2 else int(sys.argv[1])

    print("="*60)
    print("Nova HTTP Benchmark - Simple Test")
    print("="*60)
    print(f"Requests per runtime: {num_requests}")
    print(f"Target URL: {URL}")

    all_results = []

    # Test Nova
    nova_cmd = r'"C:\Users\ondev\Projects\Nova\build\Release\nova.exe" run "C:\Users\ondev\Projects\Nova\benchmarks\http_hello_nova.ts"'
    nova_result = test_server("Nova", nova_cmd, num_requests)
    if nova_result:
        all_results.append(nova_result)

    time.sleep(2)

    # Test Node.js
    node_cmd = r'node "C:\Users\ondev\Projects\Nova\benchmarks\http_hello_node.js"'
    node_result = test_server("Node.js", node_cmd, num_requests)
    if node_result:
        all_results.append(node_result)

    time.sleep(2)

    # Test Bun (if available)
    try:
        subprocess.run(['bun', '--version'], capture_output=True, timeout=2)
        bun_cmd = r'bun "C:\Users\ondev\Projects\Nova\benchmarks\http_hello_bun.ts"'
        bun_result = test_server("Bun", bun_cmd, num_requests)
        if bun_result:
            all_results.append(bun_result)
    except:
        print("\nBun not available, skipping...")

    # Summary
    if len(all_results) > 0:
        print(f"\n{'='*60}")
        print("BENCHMARK SUMMARY")
        print(f"{'='*60}\n")

        print(f"{'Runtime':<12} {'RPS':>10} {'Latency (ms)':>15} {'Memory (MB)':>15}")
        print(f"{'':12} {'':10} {'Avg':>7} {'P95':>7} {'Avg':>7} {'Final':>7}")
        print("-" * 60)

        for r in all_results:
            print(f"{r['name']:<12} {r['rps']:>10.2f} {r['avg_latency']:>7.2f} {r['p95_latency']:>7.2f} "
                  f"{r['avg_memory_mb']:>7.2f} {r['final_memory_mb']:>7.2f}")

        # Find winners
        best_rps = max(all_results, key=lambda x: x['rps'])
        best_latency = min(all_results, key=lambda x: x['avg_latency'])
        best_memory = min(all_results, key=lambda x: x['avg_memory_mb'])

        print(f"\n{'='*60}")
        print("WINNERS")
        print(f"{'='*60}")
        print(f"Best Throughput: {best_rps['name']} ({best_rps['rps']:.2f} req/sec)")
        print(f"Best Latency: {best_latency['name']} ({best_latency['avg_latency']:.2f} ms)")
        print(f"Best Memory: {best_memory['name']} ({best_memory['avg_memory_mb']:.2f} MB)")

        # Speedup comparisons
        print(f"\n{'='*60}")
        print("SPEEDUP COMPARISONS (vs Nova)")
        print(f"{'='*60}")

        nova_rps = next((r['rps'] for r in all_results if r['name'] == 'Nova'), None)
        if nova_rps:
            for r in all_results:
                if r['name'] != 'Nova':
                    speedup = r['rps'] / nova_rps
                    if speedup > 1:
                        print(f"{r['name']}: {speedup:.2f}x FASTER than Nova")
                    else:
                        print(f"{r['name']}: {1/speedup:.2f}x slower than Nova (Nova is {1/speedup:.2f}x faster)")

    print(f"\n{'='*60}")
    print("Benchmark completed!")
    print(f"{'='*60}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nBenchmark interrupted by user.")
        sys.exit(1)
