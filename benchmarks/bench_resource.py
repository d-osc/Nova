#!/usr/bin/env python3
"""
Nova HTTP Resource Usage Benchmark
Measures CPU and Memory usage at similar throughput levels
"""

import subprocess
import time
import psutil
import threading
import statistics
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
import urllib.request
import urllib.error

class ResourceMonitor:
    def __init__(self, process_name):
        self.process_name = process_name
        self.samples = []
        self.running = False
        self.thread = None

    def start(self):
        self.running = True
        self.samples = []
        self.thread = threading.Thread(target=self._monitor)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()

    def _monitor(self):
        while self.running:
            try:
                # Find process by name
                for proc in psutil.process_iter(['name', 'cpu_percent', 'memory_info', 'num_threads']):
                    if proc.info['name'].lower().startswith(self.process_name.lower()):
                        cpu = proc.cpu_percent(interval=0.1)
                        memory_mb = proc.info['memory_info'].rss / (1024 * 1024)
                        threads = proc.info['num_threads']

                        self.samples.append({
                            'cpu': cpu,
                            'memory': memory_mb,
                            'threads': threads
                        })
                        break
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

            time.sleep(0.5)

    def get_stats(self):
        if not self.samples:
            return None

        cpus = [s['cpu'] for s in self.samples]
        memories = [s['memory'] for s in self.samples]
        threads = [s['threads'] for s in self.samples]

        return {
            'avg_cpu': round(statistics.mean(cpus), 2),
            'max_cpu': round(max(cpus), 2),
            'avg_memory': round(statistics.mean(memories), 2),
            'peak_memory': round(max(memories), 2),
            'avg_threads': round(statistics.mean(threads), 0),
            'samples': len(self.samples)
        }

def send_request(url, timeout=5):
    """Send a single HTTP request and measure latency"""
    try:
        start = time.time()
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=timeout) as response:
            response.read()
            latency = (time.time() - start) * 1000  # Convert to ms
            return {'success': True, 'latency': latency}
    except Exception as e:
        return {'success': False, 'error': str(e)}

def measure_throughput(url, total_requests, concurrency):
    """Measure HTTP throughput with concurrent requests"""
    print(f"  Sending {total_requests} requests with concurrency {concurrency}...")

    start_time = time.time()
    completed = 0
    errors = 0
    latencies = []

    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = [executor.submit(send_request, url) for _ in range(total_requests)]

        for future in as_completed(futures):
            result = future.result()
            if result['success']:
                completed += 1
                latencies.append(result['latency'])
            else:
                errors += 1

    duration = time.time() - start_time
    rps = completed / duration if duration > 0 else 0

    # Calculate latency percentiles
    latencies.sort()
    p50 = latencies[int(len(latencies) * 0.50)] if latencies else 0
    p90 = latencies[int(len(latencies) * 0.90)] if latencies else 0
    p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0

    return {
        'completed': completed,
        'errors': errors,
        'duration': round(duration, 2),
        'rps': round(rps, 2),
        'latency_p50': round(p50, 2),
        'latency_p90': round(p90, 2),
        'latency_p99': round(p99, 2)
    }

def benchmark_runtime(name, command, process_name, port=3000,
                     total_requests=5000, concurrency=50):
    """Benchmark a single runtime"""
    print(f"\n[{name} Benchmark]")
    print("-" * 50)
    print(f"  Starting {name} server...")

    # Start server process
    try:
        if sys.platform == 'win32':
            process = subprocess.Popen(command,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE,
                                     creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        else:
            process = subprocess.Popen(command,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE)
    except Exception as e:
        print(f"  ERROR: Failed to start server: {e}")
        return None

    # Wait for server to be ready
    url = f"http://localhost:{port}"
    max_retries = 20
    server_ready = False

    for i in range(max_retries):
        try:
            with urllib.request.urlopen(url, timeout=1) as response:
                response.read()
                server_ready = True
                print(f"  Server is ready!")
                break
        except:
            time.sleep(0.5)

    if not server_ready:
        print(f"  ERROR: Server failed to start!")
        process.terminate()
        process.wait()
        return None

    # Get initial memory
    time.sleep(1)
    initial_memory = None
    try:
        for proc in psutil.process_iter(['name', 'memory_info']):
            if proc.info['name'].lower().startswith(process_name.lower()):
                initial_memory = proc.info['memory_info'].rss / (1024 * 1024)
                print(f"  Initial Memory: {initial_memory:.2f} MB")
                break
    except:
        pass

    # Start resource monitoring
    monitor = ResourceMonitor(process_name)
    monitor.start()

    # Run throughput test
    print()
    throughput = measure_throughput(url, total_requests, concurrency)

    # Stop monitoring
    time.sleep(1)
    monitor.stop()
    resource_stats = monitor.get_stats()

    # Stop server
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()

    # Display results
    print(f"\n  === Performance Results ===")
    print(f"  Requests Completed: {throughput['completed']}")
    print(f"  Errors: {throughput['errors']}")
    print(f"  Duration: {throughput['duration']} seconds")
    print(f"  RPS: {throughput['rps']} req/sec")

    print(f"\n  === Latency ===")
    print(f"  p50: {throughput['latency_p50']} ms")
    print(f"  p90: {throughput['latency_p90']} ms")
    print(f"  p99: {throughput['latency_p99']} ms")

    if resource_stats:
        print(f"\n  === Resource Usage ===")
        print(f"  Avg CPU: {resource_stats['avg_cpu']}%")
        print(f"  Max CPU: {resource_stats['max_cpu']}%")
        print(f"  Avg Memory: {resource_stats['avg_memory']} MB")
        print(f"  Peak Memory: {resource_stats['peak_memory']} MB")
        print(f"  Avg Threads: {resource_stats['avg_threads']}")
        print(f"  Samples: {resource_stats['samples']}")

    result = {
        'name': name,
        **throughput,
        **(resource_stats if resource_stats else {})
    }

    return result

def main():
    print("=" * 50)
    print("Nova HTTP Resource Usage Benchmark")
    print("=" * 50)
    print("Total Requests: 5000")
    print("Concurrency: 50")
    print()

    results = []

    # Benchmark Nova
    import os
    nova_exe = os.path.abspath("build/Release/nova.exe")
    nova_script = os.path.abspath("benchmarks/bench_http_nova_simple.ts")

    nova_result = benchmark_runtime(
        name="Nova",
        command=[nova_exe, nova_script],
        process_name="nova",
        port=3000
    )
    if nova_result:
        results.append(nova_result)

    # Benchmark Node
    node_result = benchmark_runtime(
        name="Node",
        command=["node", "benchmarks/bench_http_node_simple.js"],
        process_name="node",
        port=3000
    )
    if node_result:
        results.append(node_result)

    # Benchmark Bun (if available)
    try:
        subprocess.run(["bun", "--version"], capture_output=True, check=True)
        bun_result = benchmark_runtime(
            name="Bun",
            command=["bun", "benchmarks/bench_http_bun_simple.ts"],
            process_name="bun",
            port=3000
        )
        if bun_result:
            results.append(bun_result)
    except:
        print("\n[Bun] Not available, skipping...")

    # Generate comparison report
    print("\n" + "=" * 50)
    print("Resource Usage Comparison")
    print("=" * 50)
    print()

    # Print table header
    print(f"{'Runtime':<10} {'RPS':>8} {'CPU%':>7} {'Memory':>10} {'Peak':>10} {'p50':>7} {'p99':>7}")
    print("-" * 70)

    for result in results:
        print(f"{result['name']:<10} "
              f"{result.get('rps', 0):>8.1f} "
              f"{result.get('avg_cpu', 0):>7.1f} "
              f"{result.get('avg_memory', 0):>10.1f} "
              f"{result.get('peak_memory', 0):>10.1f} "
              f"{result.get('latency_p50', 0):>7.2f} "
              f"{result.get('latency_p99', 0):>7.2f}")

    # Calculate efficiency metrics
    print("\n=== Efficiency Metrics ===\n")

    for result in results:
        rps = result.get('rps', 0)
        cpu = result.get('avg_cpu', 0)
        memory = result.get('avg_memory', 0)

        rps_per_cpu = round(rps / cpu, 2) if cpu > 0 else 0
        rps_per_mb = round(rps / memory, 2) if memory > 0 else 0

        print(f"{result['name']}:")
        print(f"  RPS per CPU%: {rps_per_cpu}")
        print(f"  RPS per MB: {rps_per_mb}")
        print()

    print("Benchmark completed!")

if __name__ == '__main__':
    main()
