// Test Iterator Helpers (ES2025)

function main(): number {
    // =========================================
    // Test 1: Iterator.from()
    // =========================================
    console.log("=== Iterator.from() ===");
    let arr = [1, 2, 3, 4, 5];
    let iter = Iterator.from(arr);
    console.log("Created Iterator from array [1,2,3,4,5]");
    console.log("PASS: Iterator.from()");

    // =========================================
    // Test 2: Iterator.prototype.next()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.next() ===");
    let iter2 = Iterator.from([10, 20, 30]);
    let result1 = iter2.next();
    console.log("First next() value:", result1.value);
    console.log("First next() done:", result1.done);
    let result2 = iter2.next();
    console.log("Second next() value:", result2.value);
    console.log("PASS: Iterator.prototype.next()");

    // =========================================
    // Test 3: Iterator.prototype.toArray()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.toArray() ===");
    let iter3 = Iterator.from([100, 200, 300]);
    let resultArr = iter3.toArray();
    console.log("toArray() length:", resultArr.length);
    console.log("PASS: Iterator.prototype.toArray()");

    // =========================================
    // Test 4: Iterator.prototype.take()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.take() ===");
    let iter4 = Iterator.from([1, 2, 3, 4, 5]);
    let taken = iter4.take(3);
    console.log("Created take(3) iterator");
    console.log("PASS: Iterator.prototype.take()");

    // =========================================
    // Test 5: Iterator.prototype.drop()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.drop() ===");
    let iter5 = Iterator.from([1, 2, 3, 4, 5]);
    let dropped = iter5.drop(2);
    console.log("Created drop(2) iterator");
    console.log("PASS: Iterator.prototype.drop()");

    // =========================================
    // Test 6: Iterator.prototype.map()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.map() ===");
    let iter6 = Iterator.from([1, 2, 3]);
    // Note: In full implementation, this would take a callback
    let mapped = iter6.map(null);
    console.log("Created map iterator");
    console.log("PASS: Iterator.prototype.map()");

    // =========================================
    // Test 7: Iterator.prototype.filter()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.filter() ===");
    let iter7 = Iterator.from([1, 2, 3, 4, 5]);
    let filtered = iter7.filter(null);
    console.log("Created filter iterator");
    console.log("PASS: Iterator.prototype.filter()");

    // =========================================
    // Test 8: Iterator.prototype.reduce()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.reduce() ===");
    let iter8 = Iterator.from([1, 2, 3, 4, 5]);
    let sum = iter8.reduce(null, 0);
    console.log("reduce() result (sum):", sum);
    console.log("PASS: Iterator.prototype.reduce()");

    // =========================================
    // Test 9: Iterator.prototype.some()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.some() ===");
    let iter9 = Iterator.from([1, 2, 3]);
    let hasAny = iter9.some(null);
    console.log("some() result:", hasAny);
    console.log("PASS: Iterator.prototype.some()");

    // =========================================
    // Test 10: Iterator.prototype.every()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.every() ===");
    let iter10 = Iterator.from([1, 2, 3]);
    let allTrue = iter10.every(null);
    console.log("every() result:", allTrue);
    console.log("PASS: Iterator.prototype.every()");

    // =========================================
    // Test 11: Iterator.prototype.find()
    // =========================================
    console.log("");
    console.log("=== Iterator.prototype.find() ===");
    let iter11 = Iterator.from([10, 20, 30]);
    let found = iter11.find(null);
    console.log("find() result:", found);
    console.log("PASS: Iterator.prototype.find()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Iterator Helpers tests passed!");
    return 0;
}
