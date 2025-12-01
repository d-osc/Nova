// Test Set Methods (ES2015+)

function main(): number {
    // =========================================
    // Test 1: new Set() constructor
    // =========================================
    console.log("=== new Set() ===");
    let set = new Set();
    console.log("Created new Set");
    console.log("PASS: new Set()");

    // =========================================
    // Test 2: Set.prototype.add()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.add() ===");
    set.add(1);
    set.add(2);
    set.add(3);
    set.add(1);  // Duplicate - should be ignored
    console.log("Added 4 values (1 duplicate)");
    console.log("PASS: Set.prototype.add()");

    // =========================================
    // Test 3: Set.prototype.size
    // =========================================
    console.log("");
    console.log("=== Set.prototype.size ===");
    let size = set.size;
    console.log("Set size:", size);
    console.log("PASS: Set.prototype.size");

    // =========================================
    // Test 4: Set.prototype.has()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.has() ===");
    let has1 = set.has(1);
    console.log("set.has(1):", has1);
    let has99 = set.has(99);
    console.log("set.has(99):", has99);
    console.log("PASS: Set.prototype.has()");

    // =========================================
    // Test 5: Set.prototype.delete()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.delete() ===");
    let deleted = set.delete(2);
    console.log("set.delete(2):", deleted);
    let sizeAfterDelete = set.size;
    console.log("Size after delete:", sizeAfterDelete);
    console.log("PASS: Set.prototype.delete()");

    // =========================================
    // Test 6: Set.prototype.clear()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.clear() ===");
    set.clear();
    let sizeAfterClear = set.size;
    console.log("Size after clear:", sizeAfterClear);
    console.log("PASS: Set.prototype.clear()");

    // =========================================
    // Test 7: Set.prototype.values()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.values() ===");
    let valuesSet = new Set();
    valuesSet.add(10);
    valuesSet.add(20);
    valuesSet.add(30);
    let values = valuesSet.values();
    console.log("Got values array");
    console.log("PASS: Set.prototype.values()");

    // =========================================
    // Test 8: Set.prototype.keys()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.keys() ===");
    let keys = valuesSet.keys();
    console.log("Got keys array (same as values for Set)");
    console.log("PASS: Set.prototype.keys()");

    // =========================================
    // Test 9: Set.prototype.entries()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.entries() ===");
    let entries = valuesSet.entries();
    console.log("Got entries array ([value, value] pairs)");
    console.log("PASS: Set.prototype.entries()");

    // =========================================
    // Test 10: ES2025 Set.prototype.union()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.union() (ES2025) ===");
    let setA = new Set();
    setA.add(1);
    setA.add(2);
    let setB = new Set();
    setB.add(2);
    setB.add(3);
    let unionSet = setA.union(setB);
    let unionSize = unionSet.size;
    console.log("Union size (1,2 + 2,3):", unionSize);
    console.log("PASS: Set.prototype.union()");

    // =========================================
    // Test 11: ES2025 Set.prototype.intersection()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.intersection() (ES2025) ===");
    let interSet = setA.intersection(setB);
    let interSize = interSet.size;
    console.log("Intersection size (1,2 & 2,3):", interSize);
    console.log("PASS: Set.prototype.intersection()");

    // =========================================
    // Test 12: ES2025 Set.prototype.difference()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.difference() (ES2025) ===");
    let diffSet = setA.difference(setB);
    let diffSize = diffSet.size;
    console.log("Difference size (1,2 - 2,3):", diffSize);
    console.log("PASS: Set.prototype.difference()");

    // =========================================
    // Test 13: ES2025 Set.prototype.symmetricDifference()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.symmetricDifference() (ES2025) ===");
    let symDiffSet = setA.symmetricDifference(setB);
    let symDiffSize = symDiffSet.size;
    console.log("SymmetricDifference size (1,2 ^ 2,3):", symDiffSize);
    console.log("PASS: Set.prototype.symmetricDifference()");

    // =========================================
    // Test 14: ES2025 Set.prototype.isSubsetOf()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.isSubsetOf() (ES2025) ===");
    let subsetCheck = new Set();
    subsetCheck.add(1);
    let isSubset = subsetCheck.isSubsetOf(setA);
    console.log("{1}.isSubsetOf({1,2}):", isSubset);
    console.log("PASS: Set.prototype.isSubsetOf()");

    // =========================================
    // Test 15: ES2025 Set.prototype.isSupersetOf()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.isSupersetOf() (ES2025) ===");
    let isSuperset = setA.isSupersetOf(subsetCheck);
    console.log("{1,2}.isSupersetOf({1}):", isSuperset);
    console.log("PASS: Set.prototype.isSupersetOf()");

    // =========================================
    // Test 16: ES2025 Set.prototype.isDisjointFrom()
    // =========================================
    console.log("");
    console.log("=== Set.prototype.isDisjointFrom() (ES2025) ===");
    let disjointSet = new Set();
    disjointSet.add(100);
    let isDisjoint = setA.isDisjointFrom(disjointSet);
    console.log("{1,2}.isDisjointFrom({100}):", isDisjoint);
    console.log("PASS: Set.prototype.isDisjointFrom()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Set tests passed!");
    return 0;
}
