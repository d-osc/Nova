// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const getterOnly = [
        "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9",
        "lastMatch", "$&", "lastParen", "$+",
        "leftContext", "$`", "rightContext", "$'"
    ];

    for (let i = 0; i < getterOnly.length; i++) {
        const descriptor =
            Object.getOwnPropertyDescriptor(RegExp, getterOnly[i]);
        if (descriptor === undefined) return 1;
        if (typeof descriptor.get !== "function") return 2;
        if (descriptor.set !== undefined) return 3;
        if (descriptor.enumerable !== false) return 4;
        if (descriptor.configurable !== true) return 5;
    }

    const inputNames = ["input", "$_"];
    for (let i = 0; i < inputNames.length; i++) {
        const descriptor =
            Object.getOwnPropertyDescriptor(RegExp, inputNames[i]);
        if (descriptor === undefined) return 6;
        if (typeof descriptor.get !== "function") return 7;
        if (typeof descriptor.set !== "function") return 8;
        if (descriptor.enumerable !== false) return 9;
        if (descriptor.configurable !== true) return 10;
    }

    delete RegExp["$1"];
    if (Object.getOwnPropertyDescriptor(RegExp, "$1") !== undefined) {
        return 11;
    }
    return 0;
}
