// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "array:1\ntwo\ntrue"
// NOVA_EXPECT_STDOUT_CONTAINS: "object:nova\n2\ntrue"

function main(): number {
    let source: any[] = [1, "two", true];
    let [first, second, third] = source;
    console.log("array:" + first);
    console.log(second);
    console.log(third);

    let object = { name: "nova", count: 2, enabled: true };
    let { name: label, count, enabled } = object;
    console.log("object:" + label);
    console.log(count);
    console.log(enabled);
    return 0;
}
