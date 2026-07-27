// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test generator functions: basic yield, done/value protocol, yield* delegation

function* range(start: number, end: number) {
    for (let i = start; i < end; i++) {
        yield i;
    }
}

function* counter() {
    yield 1;
    yield 2;
    yield 3;
}

function* delegated() {
    yield 0;
    yield* range(10, 13);
    yield 99;
}

function* infiniteCounter() {
    let i = 0;
    while (true) {
        yield i;
        i++;
        if (i > 5) return;
    }
}

function main(): number {
    // Basic generator
    const gen = counter();
    const first = gen.next();
    if (first.value !== 1 || first.done !== false) return 1;
    if (gen.next().value !== 2) return 2;
    if (gen.next().value !== 3) return 3;
    const done = gen.next();
    if (done.done !== true) return 4;

    // for...of on generator
    let sum = 0;
    for (const v of counter()) {
        sum += v;
    }
    if (sum !== 6) return 5;  // 1+2+3

    // range generator
    const collected: number[] = [];
    for (const v of range(5, 10)) {
        collected.push(v);
    }
    if (collected.length !== 5) return 6;
    if (collected[0] !== 5 || collected[4] !== 9) return 7;

    // yield* delegation
    const delegatedVals: number[] = [];
    for (const v of delegated()) {
        delegatedVals.push(v);
    }
    // Expected: 0, 10, 11, 12, 99
    if (delegatedVals.length !== 5) return 8;
    if (delegatedVals[0] !== 0) return 9;
    if (delegatedVals[1] !== 10) return 10;
    if (delegatedVals[2] !== 11) return 11;
    if (delegatedVals[3] !== 12) return 12;
    if (delegatedVals[4] !== 99) return 13;

    // Spread generator into array
    const arr = [...counter()];
    if (arr.length !== 3 || arr[0] !== 1 || arr[2] !== 3) return 14;

    // Infinite-style with early termination
    const infVals: number[] = [];
    for (const v of infiniteCounter()) {
        infVals.push(v);
    }
    if (infVals.length !== 6) return 15;  // 0,1,2,3,4,5
    if (infVals[5] !== 5) return 16;

    return 0;
}
