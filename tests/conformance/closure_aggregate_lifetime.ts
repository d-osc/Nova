// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "closure-aggregate:2,3"

function makeBoxCounter() {
    let state = { value: 1 };
    return function(): number {
        state.value = state.value + 1;
        return state.value;
    };
}

function main(): number {
    let next = makeBoxCounter();
    let first: number = next();
    let second: number = next();
    console.log("closure-aggregate:" + first + "," + second);
    return 0;
}
