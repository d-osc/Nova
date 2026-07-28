// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* range(start, end) {
    for (let value = start; value < end; value++) {
        yield value;
    }
    return end;
}

function main() {
    const iterator = range(1, 3);
    const first = iterator.next();
    const second = iterator.next();
    const done = iterator.next();
    if (first.value !== 1 || first.done !== false) return 1;
    if (second.value !== 2 || second.done !== false) return 2;
    if (done.value !== 3 || done.done !== true) return 3;
    return 0;
}
