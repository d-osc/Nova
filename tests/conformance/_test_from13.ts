// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);
    console.log("object.first=" + object.first);
    if (object.first != 30) return 1;
    console.log("object.second=" + object.second);
    if (object.second != "Nova") return 2;
    if (object["first"] != 30) return 3;
    console.log("hasOwn first");
    if (!Object.hasOwn(object, "first")) return 4;
    console.log("calling keys");
    let keys = Object.keys(object);
    console.log("keys.length=" + keys.length);
    if (keys.length != 2) return 5;
    console.log("keys[0]");
    if (keys[0] != "first") return 6;
    console.log("keys[1]");
    if (keys[1] != "second") return 7;
    return 0;
}
