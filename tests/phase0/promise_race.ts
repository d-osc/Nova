// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "race:fast"

function main(): number {
    Promise.race([Promise.resolve("fast"), Promise.reject("slow")])
        .then((value: string) => console.log("race:" + value));
    return 0;
}
