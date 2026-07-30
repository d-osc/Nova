// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"]
    ]);
    console.log("after fromEntries");
    console.log("object.first=" + object.first);
    console.log("object.second=" + object.second);
    return 0;
}
