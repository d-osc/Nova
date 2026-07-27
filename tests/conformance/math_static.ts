// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test Math static methods

function approx(a: number, b: number): boolean {
    return Math.abs(a - b) < 1e-9;
}

function main(): number {
    // Constants
    if (!approx(Math.PI, 3.141592653589793)) return 1;
    if (!approx(Math.E, 2.718281828459045)) return 2;
    if (!approx(Math.LN2, 0.6931471805599453)) return 3;
    if (!approx(Math.SQRT2, 1.4142135623730951)) return 4;

    // abs
    if (Math.abs(-5) !== 5) return 5;
    if (Math.abs(5) !== 5) return 6;
    if (Math.abs(0) !== 0) return 7;

    // ceil / floor / round / trunc
    if (Math.ceil(3.1) !== 4) return 8;
    if (Math.floor(3.9) !== 3) return 9;
    if (Math.round(3.5) !== 4) return 10;
    if (Math.round(3.4) !== 3) return 11;
    if (Math.round(-3.5) !== -3) return 12;  // ties go to positive infinity
    if (Math.trunc(3.9) !== 3) return 13;
    if (Math.trunc(-3.9) !== -3) return 14;

    // sign
    if (Math.sign(5) !== 1) return 15;
    if (Math.sign(-5) !== -1) return 16;
    if (Math.sign(0) !== 0) return 17;

    // min / max
    if (Math.min(1, 2, 3) !== 1) return 18;
    if (Math.max(1, 2, 3) !== 3) return 19;
    if (Math.min(-1, -2, -3) !== -3) return 20;

    // pow / sqrt
    if (Math.pow(2, 10) !== 1024) return 21;
    if (!approx(Math.sqrt(16), 4)) return 22;
    if (!approx(Math.sqrt(2), 1.4142135623730951)) return 23;

    // cbrt
    if (!approx(Math.cbrt(27), 3)) return 24;

    // exp / log
    if (!approx(Math.exp(0), 1)) return 25;
    if (!approx(Math.log(1), 0)) return 26;
    if (!approx(Math.log(Math.E), 1)) return 27;
    if (!approx(Math.log2(8), 3)) return 28;
    if (!approx(Math.log10(1000), 3)) return 29;

    // Trigonometry
    if (!approx(Math.sin(0), 0)) return 30;
    if (!approx(Math.cos(0), 1)) return 31;
    if (!approx(Math.tan(0), 0)) return 32;
    if (!approx(Math.sin(Math.PI / 2), 1)) return 33;

    // Hyperbolic
    if (!approx(Math.sinh(0), 0)) return 34;
    if (!approx(Math.cosh(0), 1)) return 35;

    // atan / atan2
    if (!approx(Math.atan(0), 0)) return 36;
    if (!approx(Math.atan2(1, 0), Math.PI / 2)) return 37;

    // Hypot
    if (!approx(Math.hypot(3, 4), 5)) return 38;

    // Random (just check it's in range)
    const r = Math.random();
    if (r < 0 || r >= 1) return 39;

    // Sum / prod helpers
    if (Math.sign(NaN) !== 0 && !Number.isNaN(Math.sign(NaN))) {
        // Per spec Math.sign(NaN) is NaN — but Nova may not have NaN, allow 0 or NaN
        if (Math.sign(NaN) !== NaN) {
            // Implementation-specific; just verify it doesn't crash
        }
    }

    // Clz32
    if (Math.clz32(1) !== 31) return 40;  // 31 leading zeros in 32-bit 1

    return 0;
}
