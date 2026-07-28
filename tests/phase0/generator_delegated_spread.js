// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* range(start, end) {
    for (let value = start; value < end; value++) yield value;
}

function* delegated() {
    yield 0;
    yield* range(1, 4);
}

function main() {
    const values = [...delegated()];
    return values.join(",") === "0,1,2,3" ? 0 : 1;
}
