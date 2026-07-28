// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* range(start, end) {
    for (let value = start; value < end; value++) yield value;
}

function main() {
    return range(2, 3).next().value === 2 ? 0 : 1;
}
