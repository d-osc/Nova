// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (!Number.isNaN(NaN)) return 1;
    if (Number.isNaN(1.5)) return 2;
    if (Number.isNaN("NaN")) return 3;

    if (!Number.isFinite(0)) return 4;
    if (!Number.isFinite(-12.5)) return 5;
    if (Number.isFinite(NaN)) return 6;
    if (Number.isFinite(Infinity)) return 7;
    if (Number.isFinite("12")) return 8;

    if (!Number.isInteger(12)) return 9;
    if (!Number.isInteger(12.0)) return 10;
    if (Number.isInteger(12.5)) return 11;
    if (Number.isInteger(NaN)) return 12;
    if (Number.isInteger("12")) return 13;

    if (!Number.isSafeInteger(9007199254740991)) return 14;
    if (!Number.isSafeInteger(-9007199254740991)) return 15;
    if (Number.isSafeInteger(9007199254740992)) return 16;
    if (Number.isSafeInteger(1.25)) return 17;
    if (Number.isSafeInteger(Infinity)) return 18;

    return 0;
}
