// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class Counter {
    value: number;

    constructor(initial: number) {
        this.value = initial;
    }

    increment(): number {
        this.value += 1;
        return this.value;
    }
}

function main(): number {
    let counter = new Counter(40);
    if (counter.increment() != 41) return 1;
    if (counter.increment() != 42) return 2;
    return 0;
}
