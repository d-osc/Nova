// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (!Object.is(NaN, NaN)) return 1;
    if (!Object.is(0.0, 0.0)) return 2;
    if (Object.is(0.0, -0.0)) return 3;
    if (!Object.is(-0.0, -0.0)) return 4;
    if (!Object.is(1, 1.0)) return 5;
    if (Object.is(1, 2.0)) return 6;

    let object = { value: 1 };
    let alias = object;
    let other = { value: 1 };
    if (!Object.is(object, alias)) return 7;
    if (Object.is(object, other)) return 8;

    let array = [1, 2, 3];
    let arrayAlias = array;
    let otherArray = [1, 2, 3];
    if (!Object.is(array, arrayAlias)) return 9;
    if (Object.is(array, otherArray)) return 10;

    return 0;
}
