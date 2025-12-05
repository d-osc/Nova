#!/bin/bash
# Run SQLite Benchmarks - Compare Nova Standard vs Ultra vs Node.js

echo "========================================"
echo "SQLite Performance Comparison Benchmark"
echo "========================================"
echo ""

# Check if nova compiler exists
if [ ! -f "../build/Release/nova" ] && [ ! -f "../build/windows-native/nova.exe" ]; then
    echo "Error: Nova compiler not found!"
    echo "Please build Nova first with: cmake --build build --config Release"
    exit 1
fi

# Determine which binary to use
if [ -f "../build/Release/nova" ]; then
    NOVA="../build/Release/nova"
elif [ -f "../build/windows-native/nova.exe" ]; then
    NOVA="../build/windows-native/nova.exe"
fi

echo "Using Nova compiler: $NOVA"
echo ""

# Run Node.js benchmark (if available)
if command -v node &> /dev/null; then
    echo "========================================="
    echo "Node.js SQLite Benchmark"
    echo "========================================="
    node sqlite_benchmark.ts
    echo ""
else
    echo "Node.js not found - skipping Node.js benchmark"
    echo ""
fi

# Run Nova standard benchmark
echo "========================================="
echo "Nova Standard SQLite Benchmark"
echo "========================================="
$NOVA sqlite_benchmark.ts
echo ""

# Run Nova ultra benchmark
echo "========================================="
echo "Nova ULTRA SQLite Benchmark"
echo "========================================="
$NOVA sqlite_ultra_benchmark.ts
echo ""

echo "========================================="
echo "Benchmark Complete!"
echo "========================================="
echo ""
echo "Expected Results:"
echo "  Nova Ultra vs Node.js:"
echo "    • Batch Insert: 5-10x faster"
echo "    • Repeated Queries: 5-7x faster"
echo "    • Large Results: 4-5x faster"
echo "    • Memory Usage: 50-70% reduction"
echo ""
echo "  Nova Ultra vs Nova Standard:"
echo "    • Statement Caching: 3-5x faster"
echo "    • Connection Pooling: 2-3x faster"
echo "    • Zero-Copy Strings: 2-4x faster"
echo "========================================="
