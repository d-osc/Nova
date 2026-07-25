// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "array-param:1/two/1/true"

function readArray([first, second, ...rest]): number {
    console.log("array-param:" + first + "/" + second + "/" + rest.length + "/" + rest[0]);
    return 0;
}

function main(): number {
    readArray([1, "two", true]);
    return 0;
}
