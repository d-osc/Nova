// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "closure-count:2"
// NOVA_EXPECT_STDOUT_CONTAINS: "closure-object:2"

function main(): number {
    let count: number = 0;
    let state = { value: 0 };
    function increment(): number {
        count = count + 1;
        state.value = state.value + 1;
        return count;
    }
    increment();
    increment();
    console.log("closure-count:" + count);
    console.log("closure-object:" + state.value);
    return 0;
}
