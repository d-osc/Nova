// Comprehensive test for all fixes

// Test 1: Ternary operator with strings and numbers
const ternary1 = 5 > 3 ? "yes" : "no";
const ternary2 = 10 < 5 ? 100 : 200;
const value = 15;
const ternary3 = value > 20 ? "big" : value > 10 ? "medium" : "small";

console.log("=== Ternary Operator Tests ===");
console.log("5 > 3 ? 'yes' : 'no' =", ternary1, "Expected: yes");
console.log("10 < 5 ? 100 : 200 =", ternary2, "Expected: 200");
console.log("Nested ternary =", ternary3, "Expected: medium");

// Test 2: Class with string fields
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }

    greet() {
        return "Hello";
    }

    getAge() {
        return 25;
    }
}

const person = new Person("Alice", 25);

console.log("\n=== Class Field Tests ===");
console.log("Person name:", person.name, "Expected: Alice");
console.log("Person age:", person.age, "Expected: 25");

console.log("\n=== Method Call Tests ===");
console.log("person.greet():", person.greet(), "Expected: Hello");
console.log("person.getAge():", person.getAge(), "Expected: 25");

console.log("\n=== All Tests Complete ===");
