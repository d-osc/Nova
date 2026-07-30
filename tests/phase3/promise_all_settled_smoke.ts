// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "settled"

function main(): number {
    Promise.allSettled([
        Promise.resolve(1),
        Promise.reject("bad"),
        Promise.resolve(3)
    ]).then(() => {
        console.log("settled");
    });
    return 0;
}
