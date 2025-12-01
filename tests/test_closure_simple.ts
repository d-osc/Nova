// Test basic function features (closure syntax is supported but limited)

function add(a: number, b: number): number {
    return a + b;
}

function main(): number {
    // Test basic function call
    let result: number = add(10, 5);

    if (result != 15) {
        return 1;
    }

    return 0;
}
