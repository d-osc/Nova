// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "function-call:23"
// NOVA_EXPECT_STDOUT_CONTAINS: "function-apply:45"
// NOVA_EXPECT_STDOUT_CONTAINS: "function-apply-variable:89"
// NOVA_EXPECT_STDOUT_CONTAINS: "function-bind:67"

function combine(a: number, b: number): number {
    return a * 10 + b;
}

function main(): number {
    let called: number = combine.call(null, 2, 3);
    let applied: number = combine.apply(null, [4, 5]);
    let applyArguments = [8, 9];
    let appliedVariable: number = combine.apply(null, applyArguments);
    let prefix: number = 6;
    let bound = combine.bind(null, prefix);
    prefix = 8;
    let boundResult: number = bound(7);
    console.log("function-call:" + called);
    console.log("function-apply:" + applied);
    console.log("function-apply-variable:" + appliedVariable);
    console.log("function-bind:" + boundResult);
    return 0;
}
