// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "array-rest:1/2/two/true"

function main(): number {
    let [head, ...tail] = [1, "two", true];
    console.log("array-rest:" + head + "/" + tail.length + "/" + tail[0] + "/" + tail[1]);
    return 0;
}
