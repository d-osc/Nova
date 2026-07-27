// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    if (eval("1 + 2") !== 3) return 1;

    let local = 1;
    eval("local = 7");
    if (local !== 7) return 2;

    const add = new Function("left", "right", "return left + right");
    if (add(2, 3) !== 5) return 3;

    if (globalThis.globalThis !== globalThis) return 4;
    if (typeof globalThis.JSON !== "object") return 5;

    const indirectEval = eval;
    globalThis.dynamicValue = 9;
    if (indirectEval("dynamicValue") !== 9) return 6;
    delete globalThis.dynamicValue;

    return 0;
}
