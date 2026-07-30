// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* values() {
    yield 1;
    yield 2;
    yield 3;
}

function main(): number {
    let total = 0;
    for (const value of values()) {
        total += value;
    }
    return total === 6 ? 0 : 1;
}
