// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let entries = [["first", 10]];
    console.log("entries.length=" + entries.length);
    let obj = Object.fromEntries(entries);
    console.log("after fromEntries");
    console.log("obj.first=" + obj.first);
    return 0;
}
