// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
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
    if (total !== 9) return 1;
    return Array.from(iterable).join(",") === "2,3,4" ? 0 : 2;
}
