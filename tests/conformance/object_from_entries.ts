// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);

    if (object.first != 30) return 1;
    if (object.second != "Nova") return 2;
    if (object["first"] != 30) return 3;
    if (!Object.hasOwn(object, "first")) return 4;

    let keys = Object.keys(object);
    if (keys.length != 2) return 5;
    if (keys[0] != "first") return 6;
    if (keys[1] != "second") return 7;

    let empty = Object.fromEntries([]);
    if (Object.keys(empty).length != 0) return 8;

    return 0;
}
