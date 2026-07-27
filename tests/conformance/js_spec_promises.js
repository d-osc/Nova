// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "order:sync,promise-1,promise-2"
// NOVA_EXPECT_STDOUT_CONTAINS: "finally:value"
// NOVA_EXPECT_STDOUT_CONTAINS: "all:3"

function main() {
    const order = [];
    order.push("sync");

    Promise.resolve()
        .then(() => {
            order.push("promise-1");
            return Promise.resolve();
        })
        .then(() => {
            order.push("promise-2");
            console.log("order:" + order.join(","));
        });

    Promise.resolve("value")
        .finally(() => "ignored")
        .then((value) => console.log("finally:" + value));

    Promise.all([Promise.resolve(1), 2])
        .then((values) => console.log("all:" + (values[0] + values[1])));

    const thenable = {
        then(resolve) { resolve(42); }
    };
    Promise.resolve(thenable).then((value) => {
        if (value !== 42) throw new Error("thenable assimilation failed");
    });

    return 0;
}
