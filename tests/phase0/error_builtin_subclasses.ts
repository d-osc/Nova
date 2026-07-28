// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    try {
        throw new TypeError("wrong type");
    } catch (error) {
        if (!(error instanceof TypeError)) return 1;
        if (!(error instanceof Error)) return 2;
        if ((error as TypeError).message !== "wrong type") return 3;
    }

    try {
        throw new RangeError("out of range");
    } catch (error) {
        if (!(error instanceof RangeError)) return 4;
        if (!(error instanceof Error)) return 5;
    }
    return 0;
}
