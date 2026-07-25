// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "nested:3/4/ok"

function main(): number {
    let nested = { point: [3, 4], meta: { label: "ok" } };
    let { point: [x, y], meta: { label: nestedLabel } } = nested;
    console.log("nested:" + x + "/" + y + "/" + nestedLabel);
    return 0;
}
