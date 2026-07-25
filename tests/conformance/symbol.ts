// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let first = Symbol("token");
    let second = Symbol("token");
    if (!(first !== second)) return 1;
    if (!(typeof first === "symbol")) return 2;
    if (!(first.description === "token")) return 3;
    if (!(first.toString() === "Symbol(token)")) return 4;
    if (!(first.valueOf() === first)) return 5;

    let registeredA = Symbol.for("shared");
    let registeredB = Symbol.for("shared");
    if (!(registeredA === registeredB)) return 6;
    if (!(Symbol.keyFor(registeredA) === "shared")) return 7;

    let iteratorA = Symbol.iterator;
    let iteratorB = Symbol.iterator;
    if (!(iteratorA === iteratorB)) return 8;
    if (!(typeof iteratorA === "symbol")) return 9;
    return 0;
}
