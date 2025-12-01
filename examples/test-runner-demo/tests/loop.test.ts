// Loop tests - bun:test / jest style

function expect(actual: number, expected: number): number {
    if (actual === expected) return 0;
    return 1;
}

function main(): number {
    let fail = 0;

    // describe("For loops")
    let sum = 0;
    for (let i = 1; i <= 5; i++) {
        sum = sum + i;
    }
    fail = fail + expect(sum, 15);         // test: sum 1 to 5

    let sum2 = 0;
    for (let i = 1; i <= 10; i++) {
        sum2 = sum2 + i;
    }
    fail = fail + expect(sum2, 55);        // test: sum 1 to 10

    // describe("While loops")
    let count = 0;
    let j = 0;
    while (j < 10) {
        count = count + 1;
        j = j + 1;
    }
    fail = fail + expect(count, 10);       // test: count to 10

    // describe("Nested loops")
    let total = 0;
    for (let a = 0; a < 3; a++) {
        for (let b = 0; b < 3; b++) {
            total = total + 1;
        }
    }
    fail = fail + expect(total, 9);        // test: 3x3 nested

    let total2 = 0;
    for (let a = 0; a < 4; a++) {
        for (let b = 0; b < 5; b++) {
            total2 = total2 + 1;
        }
    }
    fail = fail + expect(total2, 20);      // test: 4x5 nested

    // describe("Do-while loops")
    let doCount = 0;
    let x = 0;
    do {
        doCount = doCount + 1;
        x = x + 1;
    } while (x < 3);
    fail = fail + expect(doCount, 3);      // test: do-while

    return fail;
}
