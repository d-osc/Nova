// Test async function declaration (ES2017)
// Note: Compiles as synchronous function (async runtime not yet available)

async function asyncAdd(a: number, b: number): number {
    return a + b;
}

async function asyncMultiply(x: number, y: number): number {
    let result: number = x * y;
    return result;
}

function main(): number {
    // Test async function (runs synchronously)
    let sum: number = asyncAdd(10, 20);
    if (sum != 30) {
        return 1;
    }

    let product: number = asyncMultiply(6, 7);
    if (product != 42) {
        return 2;
    }

    return 0;  // Success
}
