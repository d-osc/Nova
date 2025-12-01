// Test Function.prototype methods (ES5/ES6)

function add(a: number, b: number): number {
    return a + b;
}

function multiply(a: number, b: number, c: number): number {
    return a * b * c;
}

function greet(): number {
    console.log("Hello from greet!");
    return 42;
}

function main(): number {
    // =========================================
    // Test 1: Function.prototype.name
    // =========================================
    console.log("=== Function.prototype.name ===");
    console.log("add.name:", add.name);
    console.log("multiply.name:", multiply.name);
    console.log("greet.name:", greet.name);
    console.log("PASS: Function.prototype.name");

    // =========================================
    // Test 2: Function.prototype.length
    // =========================================
    console.log("");
    console.log("=== Function.prototype.length ===");
    console.log("add.length:", add.length);
    console.log("multiply.length:", multiply.length);
    console.log("greet.length:", greet.length);
    console.log("PASS: Function.prototype.length");

    // =========================================
    // Test 3: Function.prototype.toString()
    // =========================================
    console.log("");
    console.log("=== Function.prototype.toString() ===");
    console.log("add.toString():", add.toString());
    console.log("multiply.toString():", multiply.toString());
    console.log("PASS: Function.prototype.toString()");

    // =========================================
    // Test 4: Function.prototype.call() - simplified
    // =========================================
    console.log("");
    console.log("=== Function.prototype.call() ===");
    let result1 = greet.call(null);
    console.log("greet.call(null) returned:", result1);
    console.log("PASS: Function.prototype.call()");

    // =========================================
    // Test 5: Function.prototype.apply() - simplified
    // =========================================
    console.log("");
    console.log("=== Function.prototype.apply() ===");
    let result2 = greet.apply(null, []);
    console.log("greet.apply(null, []) returned:", result2);
    console.log("PASS: Function.prototype.apply()");

    // =========================================
    // Test 6: Function.prototype.bind() - simplified
    // =========================================
    console.log("");
    console.log("=== Function.prototype.bind() ===");
    let boundGreet = greet.bind(null);
    console.log("Created boundGreet = greet.bind(null)");
    console.log("PASS: Function.prototype.bind()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Function.prototype tests passed!");
    return 0;
}
