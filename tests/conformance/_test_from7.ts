// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"]
    ]);
    let keys = Object.keys(object);
    console.log("after keys");
    return 0;
}
