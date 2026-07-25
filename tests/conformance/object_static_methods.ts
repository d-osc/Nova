// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = { first: 10, second: 20, third: 30 };

    let keys = Object.keys(object);
    if (keys.length != 3) return 1;
    if (keys[0] != "first") return 7;
    if (keys[1] != "second") return 8;
    if (keys[2] != "third") return 9;

    let values = Object.values(object);
    if (values.length != 3) return 2;
    if (values[0] != 10) return 3;
    if (values[1] != 20) return 4;
    if (values[2] != 30) return 5;

    let entries = Object.entries(object);
    if (entries.length != 3) return 6;
    if (entries[0][1] != 10) return 10;
    if (entries[1][1] != 20) return 11;
    if (entries[2][1] != 30) return 12;
    return 0;
}
