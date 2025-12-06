# Node.js Compatibility Test Suite

Complete test suite for comparing Nova compiler output with Node.js behavior.

## Test Files

### Core JavaScript Features
1. **01_basic_types.js** - Basic data types (numbers, strings, booleans, null, undefined)
2. **02_arithmetic.js** - Arithmetic operations and operators
3. **03_string_methods.js** - String methods and template literals
4. **04_array_methods.js** - Array methods (map, filter, reduce, etc.)
5. **05_object_operations.js** - Object creation and manipulation
6. **06_math_operations.js** - Math object methods
7. **07_json_operations.js** - JSON stringify and parse
8. **08_control_flow.js** - If/else, loops, switch statements
9. **09_functions.js** - Functions, closures, arrow functions
10. **10_comparison_operators.js** - Comparison and logical operators

## Running Tests

### With Node.js
```bash
node node_test_case/01_basic_types.js
```

### With Nova
```bash
build/Release/nova.exe run node_test_case/01_basic_types.js
```

### Run All Tests
```bash
# Node.js
node node_test_case/run_all_tests.js

# Nova
build/Release/nova.exe run node_test_case/run_all_tests.js
```

## Test Results

All tests are designed to:
- Output clear, comparable results
- Work identically in both Node.js and Nova
- Test fundamental JavaScript features
- Verify compatibility between runtimes

## Success Criteria

A test passes if:
✓ No syntax errors
✓ No runtime errors
✓ Output matches expected results
✓ Behavior is identical between Node.js and Nova
