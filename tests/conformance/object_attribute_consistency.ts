// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let computed = { first: 1, second: 2 };
    Object.defineProperty(computed, "first", { writable: false });
    computed["first"] = 10;
    computed["second"] = 20;
    if (computed.first != 1) return 1;
    if (computed.second != 20) return 2;

    let source = { visible: 10, hidden: 20 };
    Object.defineProperty(source, "hidden", { enumerable: false });
    let target = { visible: 1, hidden: 2 };
    Object.assign(target, source);
    if (target.visible != 10) return 3;
    if (target.hidden != 2) return 4;

    let protectedTarget = { first: 1, second: 2 };
    Object.defineProperty(protectedTarget, "first", { writable: false });
    let assignResult = Object.assign(
        protectedTarget,
        { first: 10, second: 20 }
    );
    if (assignResult.first != 1) return 5;
    if (assignResult.second != 20) return 6;

    assignResult["first"] = 99;
    if (protectedTarget.first != 1) return 7;

    return 0;
}
