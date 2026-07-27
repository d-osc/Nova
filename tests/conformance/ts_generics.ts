// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test generics via type erasure

function id<T>(x: T): T {
    return x;
}

function first<T>(arr: T[]): T {
    return arr[0];
}

function pair<A, B>(a: A, b: B): A {
    return a;
}

function main(): number {
    // Basic generic
    const a = id<number>(42);
    if (a !== 42) return 1;

    // Generic without type argument (inferred)
    const b = id(99);
    if (b !== 99) return 2;

    // Generic with string
    const s = id<string>("hello");
    if (s !== "hello") return 3;

    // Generic array access
    const arr = [1, 2, 3];
    const f = first<number>(arr);
    if (f !== 1) return 4;

    // Multiple type params
    const p = pair<number, string>(10, "x");
    if (p !== 10) return 5;

    return 0;
}
