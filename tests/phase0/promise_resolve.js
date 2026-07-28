// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "resolved:2"

function main() {
    Promise.resolve(1)
        .then((value) => Promise.resolve(value + 1))
        .then((value) => console.log("resolved:" + value));
    return 0;
}
