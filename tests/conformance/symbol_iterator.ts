// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test Symbol.iterator and basic iterables

class Range {
    start: number;
    end: number;
    step: number;
    cur: number;

    constructor(start: number, end: number, step: number) {
        this.start = start;
        this.end = end;
        this.step = step;
        this.cur = start;
    }

    reset(): void {
        this.cur = this.start;
    }

    hasNext(): boolean {
        return this.cur < this.end;
    }

    next(): number {
        const v = this.cur;
        this.cur = this.cur + this.step;
        return v;
    }
}

function main(): number {
    // Range as a manual iterator
    const r = new Range(0, 5, 1);
    const collected: number[] = [];
    while (r.hasNext()) {
        collected.push(r.next());
    }
    if (collected.length !== 5) return 1;
    if (collected[0] !== 0 || collected[4] !== 4) return 2;

    // Range with step
    const r2 = new Range(0, 10, 2);
    const stepped: number[] = [];
    while (r2.hasNext()) {
        stepped.push(r2.next());
    }
    if (stepped.length !== 5) return 3;
    if (stepped[0] !== 0 || stepped[1] !== 2 || stepped[4] !== 8) return 4;

    // Iterate twice (reset)
    r.reset();
    let sum = 0;
    while (r.hasNext()) sum = sum + r.next();
    if (sum !== 10) return 5;  // 0+1+2+3+4

    // Spread of an array
    const arr = [10, 20, 30];
    const spreadArr = [...arr];
    if (spreadArr.length !== 3 || spreadArr[0] !== 10) return 6;

    // String iteration via for-of
    let concat = "";
    for (const ch of "abc") concat = concat + ch;
    if (concat !== "abc") return 7;

    // Array for-of
    let arrSum = 0;
    for (const v of [1, 2, 3, 4]) arrSum = arrSum + v;
    if (arrSum !== 10) return 8;

    // Nested loop
    let total = 0;
    for (const i of [1, 2, 3]) {
        for (const j of [10, 20]) {
            total = total + i * j;
        }
    }
    if (total !== (10 + 20 + 20 + 40 + 30 + 60)) return 9;

    return 0;
}
