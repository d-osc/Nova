#!/bin/bash
# Simple HTTP Throughput Benchmark

echo "=== Nova HTTP Throughput Benchmark ==="
echo ""

# Start Nova server in background
echo "Starting Nova server..."
./build/Release/nova.exe benchmarks/http_bench_nova_simple.ts 2>nova_bench.err &
SERVER_PID=$!

# Wait for server to start
sleep 3

# Test if server is ready
if ! curl -s http://localhost:3000/ > /dev/null 2>&1; then
    echo "ERROR: Server failed to start"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo "Server ready. Running benchmark..."
echo ""

# Measure time for 1000 sequential requests
START=$(date +%s.%N)
for i in {1..1000}; do
    curl -s http://localhost:3000/ > /dev/null
done
END=$(date +%s.%N)

# Calculate throughput
DURATION=$(echo "$END - $START" | bc)
RPS=$(echo "scale=2; 1000 / $DURATION" | bc)

echo "Completed 1000 requests in $DURATION seconds"
echo "Requests/sec: $RPS"

# Cleanup
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "Benchmark complete!"
