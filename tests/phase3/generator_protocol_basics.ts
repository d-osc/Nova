// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* sequence() {
    const resumed = yield 1;
    yield resumed;
    return 3;
}

function main(): number {
    const iterator = sequence();
    const first = iterator.next();
    if (first.value !== 1 || first.done !== false) return 1;

    const second = iterator.next(2);
    if (second.value !== 2 || second.done !== false) return 2;

    const completed = iterator.next();
    if (completed.value !== 3 || completed.done !== true) return 3;

    const after = iterator.next();
    if (after.done !== true) return 4;
    return 0;
}
