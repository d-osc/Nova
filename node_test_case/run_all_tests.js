// Test Runner - Runs all test files
// Can be executed with both Node.js and Nova

console.log('=========================================');
console.log('   Node.js Compatibility Test Suite');
console.log('=========================================\n');

// List of test files
const tests = [
    '01_basic_types.js',
    '02_arithmetic.js',
    '03_string_methods.js',
    '04_array_methods.js',
    '05_object_operations.js',
    '06_math_operations.js',
    '07_json_operations.js',
    '08_control_flow.js',
    '09_functions.js',
    '10_comparison_operators.js'
];

console.log('Running ' + tests.length + ' test files...\n');

// Note: Since we can't use require() or child_process in Nova,
// this is a simplified test runner that lists the tests
// In a real scenario, each test would be run separately

console.log('Test Files:');
for (let i = 0; i < tests.length; i++) {
    console.log('  ' + (i + 1) + '. ' + tests[i]);
}

console.log('\n=========================================');
console.log('To run a specific test:');
console.log('  Node.js: node node_test_case/01_basic_types.js');
console.log('  Nova:    build/Release/nova.exe run node_test_case/01_basic_types.js');
console.log('=========================================\n');
