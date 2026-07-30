// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    const flattened = [1, 2, 3].flatMap(
        (value: number) => [value, value * 10]);
    if (flattened.length !== 6) return 1;
    if (flattened[0] !== 1 || flattened[1] !== 10) return 2;
    if (flattened[4] !== 3 || flattened[5] !== 30) return 3;

    const nested = [[1, 2], [3], [4, 5]].flat();
    if (nested.length !== 5) return 4;
    if (nested[0] !== 1 || nested[4] !== 5) return 5;

    const source = [3, 1, 2];
    const sorted = source.slice().sort(
        (left: number, right: number) => left - right);
    if (sorted[0] !== 1 || sorted[2] !== 3) return 6;
    if (source[0] !== 3) return 7;

    const filled = new Array<number>(3).fill(7);
    if (filled.length !== 3 || filled[2] !== 7) return 8;
    return 0;
}
