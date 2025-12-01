// Assertion matchers - bun:test / jest style

// expect(actual).toBe(expected)
function toBe(actual: number, expected: number): number {
    if (actual === expected) return 0;
    return 1;
}

// expect(actual).toBeGreaterThan(expected)
function toBeGreaterThan(actual: number, expected: number): number {
    if (actual > expected) return 0;
    return 1;
}

// expect(actual).toBeLessThan(expected)
function toBeLessThan(actual: number, expected: number): number {
    if (actual < expected) return 0;
    return 1;
}

// expect(actual).toBeTruthy()
function toBeTruthy(actual: number): number {
    if (actual) return 0;
    return 1;
}

// expect(actual).toBeFalsy()
function toBeFalsy(actual: number): number {
    if (!actual) return 0;
    return 1;
}

function main(): number {
    let fail = 0;

    // describe("toBe matcher")
    fail = fail + toBe(5, 5);
    fail = fail + toBe(100, 100);
    fail = fail + toBe(0, 0);

    // describe("toBeGreaterThan")
    fail = fail + toBeGreaterThan(10, 5);
    fail = fail + toBeGreaterThan(100, 99);

    // describe("toBeLessThan")
    fail = fail + toBeLessThan(3, 7);
    fail = fail + toBeLessThan(0, 1);

    // describe("toBeTruthy")
    fail = fail + toBeTruthy(1);
    fail = fail + toBeTruthy(42);

    // describe("toBeFalsy")
    fail = fail + toBeFalsy(0);

    // describe("Math with matchers")
    fail = fail + toBe(Math.sqrt(16), 4);
    fail = fail + toBe(Math.abs(-25), 25);
    fail = fail + toBe((10 + 5) * 2, 30);

    return fail;
}
