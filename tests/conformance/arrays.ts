// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let values = [10, 20, 30];
    if (values.length != 3) return 1;
    if (values[1] != 20) return 2;

    let newLength = values.push(40);
    if (newLength != 4) return 3;
    if (values.length != 4) return 4;
    if (values.pop() != 40) return 5;
    if (values.length != 3) return 6;
    return 0;
}
