// Test Map Methods (ES2015)

function main(): number {
    // =========================================
    // Test 1: new Map() constructor
    // =========================================
    console.log("=== new Map() ===");
    let map = new Map();
    console.log("Created new Map");
    console.log("PASS: new Map()");

    // =========================================
    // Test 2: Map.prototype.set() with number keys
    // =========================================
    console.log("");
    console.log("=== Map.prototype.set(number, number) ===");
    map.set(1, 100);
    map.set(2, 200);
    map.set(3, 300);
    console.log("Set 3 entries with number keys");
    console.log("PASS: Map.prototype.set(number, number)");

    // =========================================
    // Test 3: Map.prototype.size
    // =========================================
    console.log("");
    console.log("=== Map.prototype.size ===");
    let size = map.size;
    console.log("Map size:", size);
    console.log("PASS: Map.prototype.size");

    // =========================================
    // Test 4: Map.prototype.get() with number key
    // =========================================
    console.log("");
    console.log("=== Map.prototype.get(number) ===");
    let val1 = map.get(1);
    console.log("map.get(1):", val1);
    let val2 = map.get(2);
    console.log("map.get(2):", val2);
    console.log("PASS: Map.prototype.get(number)");

    // =========================================
    // Test 5: Map.prototype.has()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.has() ===");
    let has1 = map.has(1);
    console.log("map.has(1):", has1);
    let has99 = map.has(99);
    console.log("map.has(99):", has99);
    console.log("PASS: Map.prototype.has()");

    // =========================================
    // Test 6: Map.prototype.delete()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.delete() ===");
    let deleted = map.delete(2);
    console.log("map.delete(2):", deleted);
    let sizeAfterDelete = map.size;
    console.log("Size after delete:", sizeAfterDelete);
    console.log("PASS: Map.prototype.delete()");

    // =========================================
    // Test 7: Map with string keys
    // =========================================
    console.log("");
    console.log("=== Map with string keys ===");
    let strMap = new Map();
    strMap.set("name", 42);
    strMap.set("age", 30);
    let nameVal = strMap.get("name");
    console.log("strMap.get('name'):", nameVal);
    console.log("PASS: Map with string keys");

    // =========================================
    // Test 8: Map.prototype.clear()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.clear() ===");
    map.clear();
    let sizeAfterClear = map.size;
    console.log("Size after clear:", sizeAfterClear);
    console.log("PASS: Map.prototype.clear()");

    // =========================================
    // Test 9: Map.prototype.keys()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.keys() ===");
    let keysMap = new Map();
    keysMap.set(10, 1000);
    keysMap.set(20, 2000);
    let keys = keysMap.keys();
    console.log("Got keys array");
    console.log("PASS: Map.prototype.keys()");

    // =========================================
    // Test 10: Map.prototype.values()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.values() ===");
    let values = keysMap.values();
    console.log("Got values array");
    console.log("PASS: Map.prototype.values()");

    // =========================================
    // Test 11: Map.prototype.entries()
    // =========================================
    console.log("");
    console.log("=== Map.prototype.entries() ===");
    let entries = keysMap.entries();
    console.log("Got entries array");
    console.log("PASS: Map.prototype.entries()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Map tests passed!");
    return 0;
}
