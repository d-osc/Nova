// Math tests - bun:test / jest style

function expect(actual: number, expected: number): number {
    if (actual === expected) return 0;
    return 1;
}

function expectTrue(condition: number): number {
    if (condition) return 0;
    return 1;
}

function main(): number {
    let fail = 0;

    // describe("Basic arithmetic")
    fail = fail + expect(2 + 3, 5);        // test: addition
    fail = fail + expect(10 - 4, 6);       // test: subtraction
    fail = fail + expect(7 * 8, 56);       // test: multiplication
    fail = fail + expect(20 / 4, 5);       // test: division
    fail = fail + expect(17 % 5, 2);       // test: modulo

    // describe("Math.sqrt")
    fail = fail + expect(Math.sqrt(16), 4);
    fail = fail + expect(Math.sqrt(25), 5);
    fail = fail + expect(Math.sqrt(100), 10);

    // describe("Math.abs")
    fail = fail + expect(Math.abs(-10), 10);
    fail = fail + expect(Math.abs(5), 5);
    fail = fail + expect(Math.abs(0), 0);

    // describe("Comparisons")
    fail = fail + expectTrue(5 > 3);
    fail = fail + expectTrue(3 < 7);
    fail = fail + expectTrue(5 >= 5);
    fail = fail + expectTrue(5 <= 5);

    return fail;
}
