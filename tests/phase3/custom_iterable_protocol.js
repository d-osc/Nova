// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "iterable:2,3,4"

function* phase3Range(start, end) {
    for (let value = start; value < end; value++) {
        yield value;
    }
}

function main() {
    const iterable = {
        from: 2,
        to: 5,
        [Symbol.iterator]() {
            return phase3Range(this.from, this.to);
        }
    };

    const values = [];
    for (const value of iterable) values.push(value);
    console.log("iterable:" + values.join(","));
    return values.length === 3 &&
        values[0] === 2 && values[1] === 3 && values[2] === 4 ? 0 : 1;
}
