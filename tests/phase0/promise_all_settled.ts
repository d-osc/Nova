// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "settled:3"

function main(): number {
    Promise.allSettled([
        Promise.resolve(1),
        Promise.reject("bad"),
        Promise.resolve(3)
    ]).then((results: any[]) => console.log("settled:" + results.length));
    return 0;
}
