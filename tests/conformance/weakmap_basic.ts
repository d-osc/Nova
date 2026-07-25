// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Test WeakMap with object keys
    const weakmap = new WeakMap();

    // Test set() and get() with object key
    const key1 = { name: "key1" };
    const key2 = { name: "key2" };

    weakmap.set(key1, 100);
    if (weakmap.get(key1) != 100) return 1;

    weakmap.set(key2, 200);
    if (weakmap.get(key1) != 100) return 2;
    if (weakmap.get(key2) != 200) return 3;

    // Test has()
    if (weakmap.has(key1) != true) return 4;
    if (weakmap.has({}) != false) return 5; // Different object

    // Test update
    weakmap.set(key1, 999);
    if (weakmap.get(key1) != 999) return 6;

    // Test delete()
    if (weakmap.delete(key1) != true) return 7;
    if (weakmap.get(key1) != undefined) return 8;
    if (weakmap.has(key1) != false) return 9;

    // Test with string key (should work with object wrapper or not allowed)
    // Note: WeakMap only accepts objects as keys per spec

    return 0;
}
