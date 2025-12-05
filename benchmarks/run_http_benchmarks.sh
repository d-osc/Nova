#!/bin/bash

#################################################
# HTTP Performance Benchmark Suite
# Compares Nova vs Node.js vs Bun vs Deno
#################################################

set -e

DURATION=30
THREADS=4
CONNECTIONS=100

echo "============================================"
echo "HTTP Performance Benchmark Suite"
echo "============================================"
echo ""
echo "Configuration:"
echo "  Duration: ${DURATION}s"
echo "  Threads: ${THREADS}"
echo "  Connections: ${CONNECTIONS}"
echo ""

# Check if wrk is installed
if ! command -v wrk &> /dev/null; then
    echo "ERROR: wrk is not installed"
    echo "Install: sudo apt-get install wrk  # Ubuntu/Debian"
    echo "         brew install wrk          # macOS"
    exit 1
fi

# Function to run benchmark for a runtime
run_benchmark() {
    local runtime=$1
    local command=$2
    local port=$3
    local test_name=$4

    echo "================================================"
    echo "Testing: $test_name - $runtime"
    echo "================================================"

    # Start server in background
    $command benchmarks/$test_name.ts > /dev/null 2>&1 &
    local pid=$!

    # Wait for server to start
    sleep 2

    # Check if server started successfully
    if ! kill -0 $pid 2>/dev/null; then
        echo "ERROR: Failed to start $runtime server"
        return 1
    fi

    # Run benchmark
    echo "Running wrk benchmark..."
    wrk -t$THREADS -c$CONNECTIONS -d${DURATION}s http://127.0.0.1:$port/ 2>&1 | tee /tmp/wrk_${runtime}_${test_name}.txt

    # Extract key metrics
    local rps=$(grep "Requests/sec:" /tmp/wrk_${runtime}_${test_name}.txt | awk '{print $2}')
    local latency=$(grep "Latency" /tmp/wrk_${runtime}_${test_name}.txt | awk '{print $2}')

    echo ""
    echo "Results: $rps req/s | Latency: $latency"
    echo ""

    # Stop server
    kill $pid 2>/dev/null || true
    wait $pid 2>/dev/null || true
    sleep 1
}

# Benchmarks array
declare -A benchmarks=(
    ["http_hello_world"]=3000
    ["http_json_response"]=3001
    ["http_keep_alive"]=3002
    ["http_headers"]=3003
    ["http_large_response"]=3004
)

# Test 1: Hello World
echo ""
echo "###############################################"
echo "# Test 1: Hello World (Minimum Overhead)"
echo "###############################################"
echo ""

for runtime in nova node bun deno; do
    case $runtime in
        nova)
            if command -v nova &> /dev/null; then
                run_benchmark "Nova" "nova" 3000 "http_hello_world"
            else
                echo "SKIP: Nova not found"
            fi
            ;;
        node)
            if command -v node &> /dev/null; then
                run_benchmark "Node.js" "node" 3000 "http_hello_world"
            else
                echo "SKIP: Node.js not found"
            fi
            ;;
        bun)
            if command -v bun &> /dev/null; then
                run_benchmark "Bun" "bun" 3000 "http_hello_world"
            else
                echo "SKIP: Bun not found"
            fi
            ;;
        deno)
            if command -v deno &> /dev/null; then
                run_benchmark "Deno" "deno run --allow-net" 3000 "http_hello_world"
            else
                echo "SKIP: Deno not found"
            fi
            ;;
    esac
done

# Test 2: JSON Response
echo ""
echo "###############################################"
echo "# Test 2: JSON Response"
echo "###############################################"
echo ""

for runtime in nova node bun deno; do
    case $runtime in
        nova)
            if command -v nova &> /dev/null; then
                run_benchmark "Nova" "nova" 3001 "http_json_response"
            fi
            ;;
        node)
            if command -v node &> /dev/null; then
                run_benchmark "Node.js" "node" 3001 "http_json_response"
            fi
            ;;
        bun)
            if command -v bun &> /dev/null; then
                run_benchmark "Bun" "bun" 3001 "http_json_response"
            fi
            ;;
        deno)
            if command -v deno &> /dev/null; then
                run_benchmark "Deno" "deno run --allow-net" 3001 "http_json_response"
            fi
            ;;
    esac
done

# Test 3: Keep-Alive
echo ""
echo "###############################################"
echo "# Test 3: Keep-Alive (Connection Reuse)"
echo "###############################################"
echo ""

for runtime in nova node bun deno; do
    case $runtime in
        nova)
            if command -v nova &> /dev/null; then
                run_benchmark "Nova" "nova" 3002 "http_keep_alive"
            fi
            ;;
        node)
            if command -v node &> /dev/null; then
                run_benchmark "Node.js" "node" 3002 "http_keep_alive"
            fi
            ;;
        bun)
            if command -v bun &> /dev/null; then
                run_benchmark "Bun" "bun" 3002 "http_keep_alive"
            fi
            ;;
        deno)
            if command -v deno &> /dev/null; then
                run_benchmark "Deno" "deno run --allow-net" 3002 "http_keep_alive"
            fi
            ;;
    esac
done

echo ""
echo "============================================"
echo "Benchmark Complete!"
echo "============================================"
echo ""
echo "Results saved to /tmp/wrk_*.txt"
echo ""
