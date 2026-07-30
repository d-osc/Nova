// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    console.log("before fromEntries");
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"]
    ]);
    console.log("after fromEntries");
    return 0;
}
