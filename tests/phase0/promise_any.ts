// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "any:1"

function main(): number {
    Promise.any([Promise.reject("bad"), Promise.resolve(1)])
        .then((value: number) => console.log("any:" + value));
    return 0;
}
