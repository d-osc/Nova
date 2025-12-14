// Test additional ES6+ features that might work

console.log("=== Testing Additional ES6+ Features ===");

// 1. Exponentiation operator
console.log("\n1. Exponentiation operator:");
const power = 2 ** 3;
console.log("2 ** 3 =", power);

// 2. Bitwise operators
console.log("\n2. Bitwise operators:");
const bitwiseAnd = 5 & 3;
const bitwiseOr = 5 | 3;
const bitwiseXor = 5 ^ 3;
console.log("5 & 3 =", bitwiseAnd);
console.log("5 | 3 =", bitwiseOr);
console.log("5 ^ 3 =", bitwiseXor);

// 3. Computed property names
console.log("\n3. Computed property names:");
const key = "dynamicKey";
const obj = { [key]: 42 };
console.log("obj[key] =", obj[key]);

// 4. Method shorthand
console.log("\n4. Method shorthand:");
const obj2 = {
    greet() {
        return "Hello";
    }
};
console.log("obj2.greet() =", obj2.greet());

// 5. For-in loop
console.log("\n5. For-in loop:");
const testObj = { a: 1, b: 2, c: 3 };
for (const key in testObj) {
    console.log("key:", key);
}

console.log("\n=== Tests complete ===");
