// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Test basic Set creation
    const mySet = new Set();

    // Test size on empty set
    if (mySet.size != 0) return 1;

    // Test add()
    mySet.add(1);
    if (mySet.size != 1) return 2;

    // Test add() chaining
    mySet.add(2).add(3);
    if (mySet.size != 3) return 3;

    // Test has()
    if (mySet.has(1) != true) return 4;
    if (mySet.has(99) != false) return 5;

    // Test unique values (adding same value doesn't increase size)
    mySet.add(1);
    if (mySet.size != 3) return 6;

    // Test delete()
    if (mySet.delete(2) != true) return 7;
    if (mySet.size != 2) return 8;
    if (mySet.has(2) != false) return 9;
    if (mySet.delete(99) != false) return 10;

    // Test clear()
    mySet.clear();
    if (mySet.size != 0) return 11;

    // Test with strings
    const strSet = new Set();
    strSet.add("hello");
    strSet.add("world");
    if (strSet.size != 2) return 12;
    if (strSet.has("hello") != true) return 13;

    return 0;
}
