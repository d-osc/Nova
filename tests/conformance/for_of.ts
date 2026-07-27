// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test for...of with various iterables: arrays, strings, sets, maps, custom iterators

function main(): number {
    // Array iteration
    const arr = [10, 20, 30];
    let sum = 0;
    for (const v of arr) {
        sum += v;
    }
    if (sum !== 60) return 1;

    // String iteration
    const s = "abc";
    let concat = "";
    for (const ch of s) {
        concat += ch;
    }
    if (concat !== "abc") return 2;

    // Set iteration
    const mySet = new Set<number>();
    mySet.add(1).add(2).add(2).add(3);
    const setVals: number[] = [];
    for (const v of mySet) {
        setVals.push(v);
    }
    if (setVals.length !== 3) return 3;
    if (setVals[0] !== 1 || setVals[1] !== 2 || setVals[2] !== 3) return 4;

    // Map iteration (entries)
    const myMap = new Map<string, number>();
    myMap.set("a", 1).set("b", 2);

    // Map keys/values
    const mapKeys: string[] = [];
    for (const k of myMap.keys()) {
        mapKeys.push(k);
    }
    if (mapKeys.length !== 2 || mapKeys[0] !== "a" || mapKeys[1] !== "b") return 5;

    const mapVals: number[] = [];
    for (const v of myMap.values()) {
        mapVals.push(v);
    }
    if (mapVals.length !== 2 || mapVals[0] !== 1 || mapVals[1] !== 2) return 6;

    // break inside for...of
    let breakResult = 0;
    for (const v of [1, 2, 3, 4, 5]) {
        if (v === 3) break;
        breakResult += v;
    }
    if (breakResult !== 3) return 9;  // 1+2

    // continue inside for...of
    let contResult = 0;
    for (const v of [1, 2, 3, 4, 5]) {
        if (v % 2 === 0) continue;
        contResult += v;
    }
    if (contResult !== 9) return 10;  // 1+3+5

    // Empty array
    let count = 0;
    for (const v of []) {
        count++;
    }
    if (count !== 0) return 11;

    // Nested destructuring in for...of
    const pairs = [[1, 2], [3, 4], [5, 6]];
    let pairSum = 0;
    for (const pair of pairs) {
        const a = pair[0];
        const b = pair[1];
        pairSum += a + b;
    }
    if (pairSum !== 21) return 7;  // (1+2)+(3+4)+(5+6)

    return 0;
}

