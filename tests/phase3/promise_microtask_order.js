// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "order:sync,first,second"

function main() {
    const order = ["sync"];
    Promise.resolve(1)
        .then(() => {
            order.push("first");
        })
        .then(() => {
            order.push("second");
            console.log("order:" + order.join(","));
        });
    return 0;
}
