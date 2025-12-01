// Test simple function call
function add(a: number, b: number): number {
    return a + b;
}

function main(): number {
    let result = add(10, 20);
    return result;  // Should be 30
}
