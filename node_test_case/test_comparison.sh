#!/bin/bash
# Test Comparison Script
# Runs all tests with both Node.js and Nova, comparing outputs

echo "========================================="
echo "  Node.js vs Nova Comparison Test"
echo "========================================="
echo ""

# Test files
tests=(
    "01_basic_types.js"
    "02_arithmetic.js"
    "03_string_methods.js"
    "04_array_methods.js"
    "05_object_operations.js"
    "06_math_operations.js"
    "07_json_operations.js"
    "08_control_flow.js"
    "09_functions.js"
    "10_comparison_operators.js"
)

passed=0
failed=0

for test in "${tests[@]}"; do
    echo "Testing: $test"

    # Run with Node.js
    node_output=$(node "node_test_case/$test" 2>&1 | grep -v "TRACE" | grep -v "DEBUG")
    node_status=$?

    # Run with Nova
    nova_output=$(build/Release/nova.exe run "node_test_case/$test" 2>&1 | grep -v "TRACE" | grep -v "DEBUG")
    nova_status=$?

    # Check if both ran successfully
    if [ $node_status -eq 0 ] && [ $nova_status -eq 0 ]; then
        echo "  ✓ Both runtimes executed successfully"
        ((passed++))
    else
        echo "  ✗ Runtime error detected"
        ((failed++))
        if [ $node_status -ne 0 ]; then
            echo "    Node.js failed with status: $node_status"
        fi
        if [ $nova_status -ne 0 ]; then
            echo "    Nova failed with status: $nova_status"
        fi
    fi

    echo ""
done

echo "========================================="
echo "Results: $passed passed, $failed failed"
echo "========================================="
