// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* range(start: number, end: number) {
    for (let value = start; value < end; value++) yield value;
}

function main(): number {
    return range(2, 3).next().value === 2 ? 0 : 1;
}
