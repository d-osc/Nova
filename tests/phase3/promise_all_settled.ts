// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "settled-length:3"
// NOVA_EXPECT_STDOUT_CONTAINS: "settled-status:fulfilled,rejected,fulfilled"

function main(): number {
    Promise.allSettled([
        Promise.resolve(1),
        Promise.reject("bad"),
        Promise.resolve(3)
    ]).then((results: any[]) => {
        console.log("settled-length:" + results.length);
        console.log(
            "settled-status:" +
            results[0].status + "," +
            results[1].status + "," +
            results[2].status
        );
    });
    return 0;
}
