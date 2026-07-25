// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (0) return 1;
    if (-0.0) return 2;
    if (NaN) return 3;
    if ("") return 4;
    if (null) return 5;
    if (undefined) return 6;

    if (!1) return 7;
    if (!0 !== true) return 8;
    if (!NaN !== true) return 9;
    if (!"" !== true) return 10;
    if (!"nova" !== false) return 11;
    if (![] !== false) return 12;
    if (!{} !== false) return 13;

    let count = 0;
    while (2.5) {
        count++;
        break;
    }
    if (count != 1) return 14;

    do {
        count++;
    } while (NaN);
    if (count != 2) return 15;

    for (; "nova"; ) {
        count++;
        break;
    }
    if (count != 3) return 16;

    if ((NaN ? 1 : 2) != 2) return 17;
    if (("" ? 1 : 2) != 2) return 18;
    if (([] ? 1 : 2) != 1) return 19;

    return 0;
}
