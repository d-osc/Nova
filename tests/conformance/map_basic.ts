// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Test basic Map creation
    const map = new Map();

    // Test size on empty map
    if (map.size != 0) return 1;

    // Test set() with string key and number value
    map.set("a", 1);
    if (map.size != 1) return 2;

    // Test set() chaining
    map.set("b", 2).set("c", 3);
    if (map.size != 3) return 3;

    // Test get()
    if (map.get("a") != 1) return 4;
    if (map.get("b") != 2) return 5;
    if (map.get("c") != 3) return 6;
    if (map.get("nonexistent") != undefined) return 7;

    // Test has()
    if (map.has("a") != true) return 8;
    if (map.has("z") != false) return 9;

    // Test update existing key
    map.set("a", 10);
    if (map.get("a") != 10) return 10;
    if (map.size != 3) return 11;

    // Test delete()
    if (map.delete("b") != true) return 12;
    if (map.size != 2) return 13;
    if (map.has("b") != false) return 14;
    if (map.delete("z") != false) return 15;

    // Test clear()
    map.clear();
    if (map.size != 0) return 16;

    // Test with number keys
    const numMap = new Map();
    numMap.set(1, "one");
    numMap.set(2, "two");
    if (numMap.get(1) != "one") return 17;
    if (numMap.size != 2) return 18;

    return 0;
}
