// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let sum = 0;
    for (let index = 0; index < 10; index++) {
        if (index == 5) continue;
        if (index == 9) break;
        sum += index;
    }
    if (sum != 31) return 1;

    let count = 3;
    while (count > 0) count--;
    if (count != 0) return 2;
    return 0;
}
