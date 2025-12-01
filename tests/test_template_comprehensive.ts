// Comprehensive template literal test
function greet(name: string): string {
    return `Hello ${name}!`;
}

function main(): number {
    // Test 1: Simple template literal with variable
    let name = "World";
    let greeting = `Hello ${name}!`;

    // Test 2: Template literal with function call
    let message = greet("Nova");

    // Test 3: Template literal with multiple expressions
    let a = "foo";
    let b = "bar";
    let combined = `${a} and ${b}`;

    // Test 4: Template literal with empty strings
    let simple = `Just a string`;

    // For now, just return success (later we can test string.length)
    return 42;
}
