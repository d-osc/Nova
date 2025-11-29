// Test using statement (ES2024 Explicit Resource Management)

function testUsing(): number {
    // Basic using statement - creates a const-like binding
    using resource = 42;

    return resource;
}

function testUsingWithExpression(): number {
    let base: number = 10;
    using computed = base + 5;

    return computed;  // Should return 15
}

function main(): number {
    let result: number = testUsing();

    if (result != 42) {
        return 1;  // Fail
    }

    result = testUsingWithExpression();
    if (result != 15) {
        return 2;  // Fail
    }

    return 0;  // Success
}
