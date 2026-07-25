// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let stringPath = 5;
    if (true) {
        stringPath = "8";
    }
    if (!(stringPath === "8")) return 1;
    if (!((stringPath + "x") === "8x")) return 2;
    if (!((stringPath - 3) === 5)) return 3;
    if (!(+stringPath === 8)) return 4;

    let numberPath = 1;
    if (false) {
        numberPath = "not selected";
    }
    if (!(numberPath === 1)) return 5;
    if (!((numberPath + 2) === 3)) return 6;
    if (!(numberPath < 2)) return 7;
    if (!((numberPath | 2) === 3)) return 8;

    let stringCompare = "a";
    if (true) stringCompare = "b";
    if (!(stringCompare > "a")) return 9;

    let loopValue = 0;
    let index = 0;
    while (index < 2) {
        if (index === 1) loopValue = "done";
        index++;
    }
    if (!(loopValue === "done")) return 10;

    let compound = 1;
    if (false) compound = "unused";
    compound += 2;
    if (!(compound === 3)) return 11;
    compound++;
    if (!(compound === 4)) return 12;
    compound -= 2;
    if (!(compound === 2)) return 13;
    compound--;
    if (!(compound === 1)) return 14;

    return 0;
}
