// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const objectKey = {};
    const map = new Map();
    map.set("first", 1);
    map.set(NaN, 2);
    map.set(objectKey, 3);
    map.set("first", 4);

    if (map.size !== 3) return 1;
    if (map.get(NaN) !== 2 || map.get(objectKey) !== 3) return 2;
    if ([...map.keys()][0] !== "first") return 3;

    const set = new Set([1, 2, 2, NaN, NaN]);
    if (set.size !== 3 || !set.has(NaN)) return 4;
    if ([...set].join(",") !== "1,2,NaN") return 5;

    const weakMap = new WeakMap();
    weakMap.set(objectKey, "value");
    if (weakMap.get(objectKey) !== "value") return 6;
    if (!weakMap.delete(objectKey) || weakMap.has(objectKey)) return 7;

    const weakSet = new WeakSet();
    const member = {};
    weakSet.add(member);
    if (!weakSet.has(member)) return 8;
    if (!weakSet.delete(member) || weakSet.has(member)) return 9;

    return 0;
}
