// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function choose(flag: boolean) {
    if (flag) return 7;
    return "seven";
}

function maybeObject(flag: boolean) {
    let object = { value: 9 };
    if (flag) return object;
    return null;
}

function identity(value) {
    return value;
}

function plusOne(value) {
    return value + 1;
}

function main(): number {
    if (!(choose(true) === 7)) return 1;
    if (!(choose(false) === "seven")) return 2;
    if (!((choose(true) + 1) === 8)) return 3;
    if (!((choose(false) + "!") === "seven!")) return 4;

    let objectResult = maybeObject(true);
    if (!objectResult) return 5;
    if (!(maybeObject(false) === null)) return 6;

    if (!(identity(4) === 4)) return 7;
    if (!(identity("four") === "four")) return 8;
    if (!((plusOne(4)) === 5)) return 9;
    if (!((plusOne("4")) === "41")) return 10;
    return 0;
}
