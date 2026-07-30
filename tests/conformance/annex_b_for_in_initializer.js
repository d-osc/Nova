// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    let effects = 0;
    let iterations = 0;
    for (var key = ++effects in { first: 1, second: 2 }) {
        iterations++;
    }
    if (effects !== 1) return 1;
    if (iterations !== 2) return 2;
    return 0;
}
