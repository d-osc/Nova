// Basic tests - bun:test / jest style

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

    // describe("Variables")
    let x = 10;
    fail = fail + expect(x, 10);           // test: assignment
    x = 20;
    fail = fail + expect(x, 20);           // test: reassignment

    let a = 5;
    let b = 3;
    fail = fail + expect(a + b, 8);        // test: multiple vars

    // describe("Arithmetic")
    fail = fail + expect(1 + 1, 2);        // test: addition
    fail = fail + expect(5 - 3, 2);        // test: subtraction
    fail = fail + expect(4 * 3, 12);       // test: multiplication
    fail = fail + expect(10 / 2, 5);       // test: division
    fail = fail + expect((2 + 3) * 4, 20); // test: complex expression

    // describe("Comparisons")
    fail = fail + expectTrue(5 === 5);     // test: equality
    fail = fail + expectTrue(5 !== 3);     // test: inequality
    fail = fail + expectTrue(10 > 5);      // test: greater than
    fail = fail + expectTrue(3 < 7);       // test: less than

    return fail;
}
