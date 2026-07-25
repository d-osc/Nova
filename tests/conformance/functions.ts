// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function add(a: number, b: number): number {
    return a + b;
}

function factorial(value: number): number {
    if (value <= 1) return 1;
    return value * factorial(value - 1);
}

function main(): number {
    if (add(20, 22) != 42) return 1;
    if (factorial(5) != 120) return 2;
    return 0;
}
