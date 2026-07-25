// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let target = { first: 1, second: 2 };
    let source = { first: 10, second: 20 };
    let result = Object.assign(target, source);

    if (result.first != 10) return 1;
    if (result.second != 20) return 2;
    if (target.first != 10) return 3;
    if (target.second != 20) return 4;
    if (!Object.hasOwn(result, "first")) return 5;
    if (Object.hasOwn(result, "missing")) return 6;
    return 0;
}
