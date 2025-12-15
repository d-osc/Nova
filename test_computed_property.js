// Computed property access
const obj = {
    name: "test",
    value: 42
};

const key1 = "name";
const key2 = "value";

console.log("obj[key1]:", obj[key1]);
console.log("obj[key2]:", obj[key2]);

console.log("\nExpected:");
console.log("obj[key1]: test");
console.log("obj[key2]: 42");
