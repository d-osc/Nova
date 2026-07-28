// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* counter() {
    yield 1;
}

function main(): number {
    return counter().next().value === 1 ? 0 : 1;
}
