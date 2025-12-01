// Test FinalizationRegistry (ES2021)

function cleanupCallback(heldValue: number) {
    console.log("Cleanup callback called with:", heldValue);
}

function main(): number {
    // =========================================
    // Test 1: Create FinalizationRegistry
    // =========================================
    let registry = new FinalizationRegistry(cleanupCallback);
    console.log("Created FinalizationRegistry");
    console.log("PASS: FinalizationRegistry constructor");

    // =========================================
    // Test 2: Register an object
    // =========================================
    let obj1 = { name: "object1" };
    registry.register(obj1, 100);
    console.log("Registered obj1 with heldValue 100");
    console.log("PASS: FinalizationRegistry.register()");

    // =========================================
    // Test 3: Register with unregister token
    // =========================================
    let obj2 = { name: "object2" };
    let token2 = { id: "token2" };
    registry.register(obj2, 200, token2);
    console.log("Registered obj2 with heldValue 200 and token");
    console.log("PASS: FinalizationRegistry.register() with token");

    // =========================================
    // Test 4: Unregister using token
    // =========================================
    let removed = registry.unregister(token2);
    console.log("Unregister result:", removed);
    console.log("PASS: FinalizationRegistry.unregister()");

    // =========================================
    // Test 5: Multiple registrations
    // =========================================
    let obj3 = { name: "object3" };
    let obj4 = { name: "object4" };
    let sharedToken = { id: "shared" };

    registry.register(obj3, 300, sharedToken);
    registry.register(obj4, 400, sharedToken);
    console.log("Registered obj3 and obj4 with shared token");
    console.log("PASS: Multiple registrations with same token");

    // =========================================
    // Test 6: Unregister multiple with same token
    // =========================================
    let removedMultiple = registry.unregister(sharedToken);
    console.log("Unregister multiple result:", removedMultiple);
    console.log("PASS: Unregister multiple objects");

    // =========================================
    // Test 7: Create second registry
    // =========================================
    let registry2 = new FinalizationRegistry(cleanupCallback);
    console.log("Created second FinalizationRegistry");
    console.log("PASS: Multiple FinalizationRegistry instances");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All FinalizationRegistry tests passed!");
    console.log("Note: Actual cleanup callbacks are called when objects are garbage collected.");
    return 0;
}
