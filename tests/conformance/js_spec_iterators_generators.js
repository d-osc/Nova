// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function* range(start, end) {
    for (let value = start; value < end; value++) {
        yield value;
    }
    return end;
}

function* delegated() {
    yield 0;
    yield* range(1, 4);
}

function main() {
    const iterator = range(1, 3);
    const first = iterator.next();
    const second = iterator.next();
    const done = iterator.next();
    if (first.value !== 1 || first.done !== false) return 1;
    if (second.value !== 2 || second.done !== false) return 2;
    if (done.value !== 3 || done.done !== true) return 3;

    const values = [...delegated()];
    if (values.join(",") !== "0,1,2,3") return 4;

    const iterable = {
        from: 2,
        to: 5,
        [Symbol.iterator]() {
            let current = this.from;
            const end = this.to;
            return {
                next() {
                    return current < end
                        ? { value: current++, done: false }
                        : { value: undefined, done: true };
                }
            };
        }
    };

    let total = 0;
    for (const value of iterable) total += value;
    if (total !== 9) return 5;
    if (Array.from(iterable).join(",") !== "2,3,4") return 6;

    return 0;
}
