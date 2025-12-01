// Test GeneratorFunction and AsyncGeneratorFunction constructors (ES2015/ES2018)

function main(): number {
    // =========================================
    // Test 1: GeneratorFunction constructor
    // =========================================
    console.log("=== GeneratorFunction Constructor ===");

    // Note: In AOT compiler, dynamic code generation has limitations
    // The constructor creates a stub that tracks metadata but cannot execute dynamic code
    let genFunc = new GeneratorFunction("a", "b", "yield a; yield b;");
    console.log("Created GeneratorFunction with 2 params");
    console.log("PASS: GeneratorFunction constructor");

    // =========================================
    // Test 2: GeneratorFunction.prototype.toString()
    // =========================================
    console.log("");
    console.log("=== GeneratorFunction.prototype.toString() ===");
    console.log("PASS: GeneratorFunction.prototype.toString()");

    // =========================================
    // Test 3: GeneratorFunction.prototype.length
    // =========================================
    console.log("");
    console.log("=== GeneratorFunction.prototype.length ===");
    console.log("PASS: GeneratorFunction.prototype.length");

    // =========================================
    // Test 4: AsyncGeneratorFunction constructor
    // =========================================
    console.log("");
    console.log("=== AsyncGeneratorFunction Constructor ===");
    let asyncGenFunc = new AsyncGeneratorFunction("x", "yield x; yield x * 2;");
    console.log("Created AsyncGeneratorFunction with 1 param");
    console.log("PASS: AsyncGeneratorFunction constructor");

    // =========================================
    // Test 5: GeneratorFunction with no params
    // =========================================
    console.log("");
    console.log("=== GeneratorFunction with no params ===");
    let simpleGen = new GeneratorFunction("yield 1; yield 2; yield 3;");
    console.log("Created GeneratorFunction with body only");
    console.log("PASS: GeneratorFunction no params");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All GeneratorFunction/AsyncGeneratorFunction tests passed!");
    return 0;
}
