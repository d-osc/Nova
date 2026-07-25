// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (5 / 2 != 2.5) return 1;
    if (1 + 2.5 != 3.5) return 2;
    if (5.5 - 2 != 3.5) return 3;
    if (1.5 * 2 != 3.0) return 4;
    if (5.5 % 2 != 1.5) return 5;
    if (4.0 ** 0.5 != 2.0) return 6;

    let sum = 0.1 + 0.2;
    if (sum < 0.2999999999999999) return 7;
    if (sum > 0.3000000000000001) return 8;
    if (!(2.5 > 2)) return 9;
    if (!(2 <= 2.0)) return 10;

    if (NaN == NaN) return 11;
    if (!(NaN != NaN)) return 12;
    if (NaN < 1) return 13;
    if (NaN >= 1) return 14;

    if (Math.PI < 3.14159265358979) return 15;
    if (Math.PI > 3.14159265358980) return 16;
    if (Math.E < 2.71828182845904) return 17;
    if (Math.LN2 < 0.69314718055994) return 18;
    if (Math.LN10 < 2.30258509299404) return 19;
    if (Math.LOG2E < 1.44269504088896) return 20;
    if (Math.LOG10E < 0.43429448190325) return 21;
    if (Math.SQRT1_2 < 0.70710678118654) return 22;
    if (Math.SQRT2 < 1.41421356237309) return 23;

    return 0;
}
