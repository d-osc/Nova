#!/bin/bash

echo "=== Testing ALL Benchmark Files ==="
echo ""

NOVA_EXE="./build/Release/nova.exe"
PASS=0
FAIL=0
SKIP=0

# Test .js files
echo "### Testing JavaScript Files (.js) ###"
echo ""

for file in benchmarks/*.js; do
    filename=$(basename "$file")

    # Skip Node.js specific files
    if [[ "$filename" == *"_node.js" ]]; then
        echo "⊘ SKIP: $filename (Node.js specific)"
        ((SKIP++))
        continue
    fi

    # Run the test
    if $NOVA_EXE "$file" > /dev/null 2>&1; then
        echo "✓ PASS: $filename"
        ((PASS++))
    else
        echo "✗ FAIL: $filename"
        ((FAIL++))
    fi
done

echo ""
echo "### Testing TypeScript Files (.ts) ###"
echo ""

for file in benchmarks/*.ts; do
    filename=$(basename "$file")

    # Skip Node.js/Bun/Deno specific files
    if [[ "$filename" == *"_node.ts" ]] || [[ "$filename" == *"_bun.ts" ]] || [[ "$filename" == *"_deno.ts" ]]; then
        echo "⊘ SKIP: $filename (Runtime specific)"
        ((SKIP++))
        continue
    fi

    # Run the test
    if $NOVA_EXE "$file" > /dev/null 2>&1; then
        echo "✓ PASS: $filename"
        ((PASS++))
    else
        echo "✗ FAIL: $filename"
        ((FAIL++))
    fi
done

echo ""
echo "=== Test Summary ==="
echo "✓ PASS: $PASS"
echo "✗ FAIL: $FAIL"
echo "⊘ SKIP: $SKIP"
echo "TOTAL: $((PASS + FAIL + SKIP))"
echo ""

PASS_RATE=$((PASS * 100 / (PASS + FAIL)))
echo "Pass Rate: $PASS_RATE% (excluding skipped)"
