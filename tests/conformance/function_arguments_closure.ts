// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "arguments-own:7"
// NOVA_EXPECT_STDOUT_CONTAINS: "arguments-arrow:8"

function makeOwnArgumentsAdder(base: number) {
    return function(value: number): number {
        return arguments[0] + base;
    };
}

function makeArrowArgumentsReader(value: number) {
    return (): number => arguments[0] + value;
}

function main(): number {
    let addThree = makeOwnArgumentsAdder(3);
    let own: number = addThree(4);
    let readOuter = makeArrowArgumentsReader(4);
    let lexical: number = readOuter();
    console.log("arguments-own:" + own);
    console.log("arguments-arrow:" + lexical);
    return 0;
}
