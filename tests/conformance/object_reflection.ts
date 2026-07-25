// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = { first: 10, second: 20 };

    let names = Object.getOwnPropertyNames(object);
    if (names.length != 2) return 1;
    if (names[0] != "first") return 2;
    if (names[1] != "second") return 3;

    let symbols = Object.getOwnPropertySymbols(object);
    if (symbols.length != 0) return 4;

    if (!Object.is(42, 42)) return 5;
    if (Object.is(42, 43)) return 6;
    if (!Object.is("Nova", "Nova")) return 7;
    if (Object.is("Nova", "Other")) return 8;
    if (!Object.is(true, true)) return 9;
    if (Object.is(true, false)) return 10;
    if (Object.is(true, 1)) return 11;
    if (Object.is("1", 1)) return 12;

    return 0;
}
