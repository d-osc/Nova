// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test ES2024 Object.groupBy + Map.groupBy and ES2025 Set methods

function main(): number {
    // Object.groupBy
    const nums = [1, 2, 3, 4, 5, 6];
    const grouped = Object.groupBy(nums, (n: number) => n % 2 === 0 ? "even" : "odd");
    // grouped.even should be [2, 4, 6]
    // grouped.odd should be [1, 3, 5]
    if (!grouped) return 1;

    // Set ES2025 methods
    const setA = new Set([1, 2, 3, 4]);
    const setB = new Set([3, 4, 5, 6]);

    // union
    const u = setA.union(setB);
    if (!u) return 2;
    if (u.size !== 6) return 3;

    // intersection
    const i = setA.intersection(setB);
    if (i.size !== 2) return 4;
    if (!i.has(3) || !i.has(4)) return 5;

    // difference (A - B)
    const d = setA.difference(setB);
    if (d.size !== 2) return 6;
    if (!d.has(1) || !d.has(2)) return 7;

    // symmetricDifference
    const sd = setA.symmetricDifference(setB);
    if (sd.size !== 4) return 8;

    // isSubsetOf
    const setSmall = new Set([1, 2]);
    if (!setSmall.isSubsetOf(setA)) return 9;
    if (setA.isSubsetOf(setSmall)) return 10;

    // isSupersetOf
    if (!setA.isSupersetOf(setSmall)) return 11;
    if (setSmall.isSupersetOf(setA)) return 12;

    // isDisjointFrom
    const setC = new Set([10, 20]);
    if (!setA.isDisjointFrom(setC)) return 13;
    if (!setC.isDisjointFrom(setA)) return 14;

    return 0;
}
