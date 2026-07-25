// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function makeAdder(base) {
    return function(value) {
        return base + value;
    };
}

function makeArrowAdder(base) {
    return (value) => base + value;
}

function main(): number {
    let addTwo = makeAdder(2);
    if (!(addTwo(3) === 5)) return 1;
    if (!(addTwo("3") === "23")) return 2;
    let addThree = makeArrowAdder(3);
    if (!(addThree(4) === 7)) return 3;
    if (!(addThree("4") === "34")) return 4;
    return 0;
}
