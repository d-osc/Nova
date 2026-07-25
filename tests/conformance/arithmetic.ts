// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let value = (8 + 6) * 3 - 5;
    if (value != 37) return 1;
    if (17 % 5 != 2) return 2;
    if (2 ** 5 != 32) return 3;
    return 0;
}
