// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "resolved:ok"

function main(): number {
    const { promise, resolve } = Promise.withResolvers();
    resolve("ok");
    promise.then((value: string) => console.log("resolved:" + value));
    return 0;
}
