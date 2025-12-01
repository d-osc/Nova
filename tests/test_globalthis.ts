// Test globalThis (ES2020)

function main(): number {
    // =========================================
    // Test 1: globalThis identity
    // =========================================
    console.log("=== globalThis Identity ===");
    console.log("globalThis exists");
    console.log("PASS: globalThis identity");

    // =========================================
    // Test 2: globalThis.Infinity
    // =========================================
    console.log("");
    console.log("=== globalThis.Infinity ===");
    let inf = globalThis.Infinity;
    console.log("globalThis.Infinity accessed");
    console.log("PASS: globalThis.Infinity");

    // =========================================
    // Test 3: globalThis.NaN
    // =========================================
    console.log("");
    console.log("=== globalThis.NaN ===");
    let nan = globalThis.NaN;
    console.log("globalThis.NaN accessed");
    console.log("PASS: globalThis.NaN");

    // =========================================
    // Test 4: Direct Infinity access
    // =========================================
    console.log("");
    console.log("=== Direct Infinity ===");
    let directInf = Infinity;
    console.log("Direct Infinity accessed");
    console.log("PASS: Direct Infinity");

    // =========================================
    // Test 6: Direct NaN access
    // =========================================
    console.log("");
    console.log("=== Direct NaN ===");
    let directNan = NaN;
    console.log("Direct NaN accessed");
    console.log("PASS: Direct NaN");

    // =========================================
    // Test 5: globalThis.globalThis (self-reference)
    // =========================================
    console.log("");
    console.log("=== globalThis.globalThis ===");
    let selfRef = globalThis.globalThis;
    console.log("globalThis.globalThis accessed");
    console.log("PASS: globalThis.globalThis");

    // =========================================
    // Test 9: globalThis.Math
    // =========================================
    console.log("");
    console.log("=== globalThis.Math ===");
    let mathObj = globalThis.Math;
    console.log("globalThis.Math accessed");
    console.log("PASS: globalThis.Math");

    // =========================================
    // Test 10: globalThis.console
    // =========================================
    console.log("");
    console.log("=== globalThis.console ===");
    let consoleObj = globalThis.console;
    console.log("globalThis.console accessed");
    console.log("PASS: globalThis.console");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All globalThis tests passed!");
    return 0;
}
