// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Test WeakSet with object values
    const weakset = new WeakSet();

    // Test add() and has() with objects
    const obj1 = { name: "obj1" };
    const obj2 = { name: "obj2" };

    weakset.add(obj1);
    if (weakset.has(obj1) != true) return 1;

    weakset.add(obj2);
    if (weakset.has(obj1) != true) return 2; // obj1 should still be there
    if (weakset.has(obj2) != true) return 3;

    // Test add() chaining
    weakset.add(obj1); // Adding same object again should be fine
    if (weakset.has(obj1) != true) return 4;

    // Test has() with non-existent object
    if (weakset.has({}) != false) return 5; // Different object instance

    // Test delete()
    if (weakset.delete(obj1) != true) return 6;
    if (weakset.has(obj1) != false) return 7;
    if (weakset.delete(obj1) != false) return 8; // Already deleted

    return 0;
}
