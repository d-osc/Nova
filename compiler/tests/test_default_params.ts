// Test default function parameters
function add(a: number, b: number = 10): number {
    return a + b;
}

function main(): number {
    let result1 = add(5, 3);   // Should be 8
    let result2 = add(5);       // Should be 15 (5 + default 10)
    return result1 + result2;   // Should be 23
}
