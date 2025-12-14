// Test more features that might work

console.log("=== Testing More Features ===");

// 1. Try-catch
console.log("\n1. Try-catch:");
try {
    console.log("In try block");
    const x = 42;
    console.log("x =", x);
} catch (e) {
    console.log("Caught error");
}

// 2. Break in loops
console.log("\n2. Break in loops:");
for (let i = 0; i < 10; i++) {
    if (i > 2) {
        break;
    }
    console.log("i =", i);
}

// 3. Continue in loops
console.log("\n3. Continue in loops:");
for (let i = 0; i < 5; i++) {
    if (i == 2) {
        continue;
    }
    console.log("i =", i);
}

// 4. Typeof operator
console.log("\n4. Typeof operator:");
const num = 42;
const str = "hello";
console.log("typeof num:", typeof num);
console.log("typeof str:", typeof str);

// 5. Logical operators with short-circuit
console.log("\n5. Short-circuit evaluation:");
const a = true;
const b = false;
console.log("true && true:", a && a);
console.log("false || true:", b || a);

console.log("\n=== Tests complete ===");
