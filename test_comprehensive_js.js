// Comprehensive JavaScript/TypeScript Feature Test
console.log("=== COMPREHENSIVE JAVASCRIPT TEST ===\n");

let totalTests = 0;
let passedTests = 0;

// Test 1: Ternary operator
totalTests = totalTests + 1;
const ternary = 5 > 3 ? "yes" : "no";
if (ternary === "yes") {
    passedTests = passedTests + 1;
    console.log("✓ 1. Ternary operator");
} else {
    console.log("✗ 1. Ternary operator FAILED");
}

// Test 2: Logical operators
totalTests = totalTests + 1;
const and = true && true;
const or = false || true;
if (and === true && or === true) {
    passedTests = passedTests + 1;
    console.log("✓ 2. Logical operators (&&, ||)");
} else {
    console.log("✗ 2. Logical operators FAILED");
}

// Test 3: Switch statement
totalTests = totalTests + 1;
let switchResult = 0;
const val = 2;
switch (val) {
    case 1:
        switchResult = 10;
        break;
    case 2:
        switchResult = 20;
        break;
    default:
        switchResult = 30;
}
if (switchResult === 20) {
    passedTests = passedTests + 1;
    console.log("✓ 3. Switch statement");
} else {
    console.log("✗ 3. Switch statement FAILED");
}

// Test 4: Arrow function with implicit return
totalTests = totalTests + 1;
const implicit = x => x * 2;
if (implicit(5) === 10) {
    passedTests = passedTests + 1;
    console.log("✓ 4. Arrow function (implicit return)");
} else {
    console.log("✗ 4. Arrow function implicit return FAILED");
}

// Test 5: Multiple variable declarations
totalTests = totalTests + 1;
const a = 1, b = 2, c = 3;
if (a === 1 && b === 2 && c === 3) {
    passedTests = passedTests + 1;
    console.log("✓ 5. Multiple variable declarations");
} else {
    console.log("✗ 5. Multiple declarations FAILED");
}

// Test 6: Nested functions
totalTests = totalTests + 1;
function outer(x) {
    function inner(y) {
        return x + y;
    }
    return inner(10);
}
if (outer(5) === 15) {
    passedTests = passedTests + 1;
    console.log("✓ 6. Nested functions");
} else {
    console.log("✗ 6. Nested functions FAILED");
}

// Test 7: Function expressions
totalTests = totalTests + 1;
const funcExpr = function(x) {
    return x * 3;
};
if (funcExpr(4) === 12) {
    passedTests = passedTests + 1;
    console.log("✓ 7. Function expressions");
} else {
    console.log("✗ 7. Function expressions FAILED");
}

// Test 8: Return early from function
totalTests = totalTests + 1;
function earlyReturn(x) {
    if (x > 10) {
        return 100;
    }
    return 50;
}
if (earlyReturn(15) === 100) {
    passedTests = passedTests + 1;
    console.log("✓ 8. Early return");
} else {
    console.log("✗ 8. Early return FAILED");
}

// Test 9: Array push/pop
totalTests = totalTests + 1;
const arr = [1, 2, 3];
arr.push(4);
if (arr[3] === 4) {
    passedTests = passedTests + 1;
    console.log("✓ 9. Array.push()");
} else {
    console.log("✗ 9. Array.push() FAILED");
}

// Test 10: String methods
totalTests = totalTests + 1;
const str = "hello";
const upper = str.toUpperCase();
if (upper === "HELLO") {
    passedTests = passedTests + 1;
    console.log("✓ 10. String.toUpperCase()");
} else {
    console.log("✗ 10. String.toUpperCase() FAILED");
}

// Test 11: Increment/Decrement
totalTests = totalTests + 1;
let counter = 10;
counter++;
if (counter === 11) {
    passedTests = passedTests + 1;
    console.log("✓ 11. Increment operator (++)");
} else {
    console.log("✗ 11. Increment operator FAILED");
}

// Test 12: Compound assignment
totalTests = totalTests + 1;
let compound = 10;
compound += 5;
if (compound === 15) {
    passedTests = passedTests + 1;
    console.log("✓ 12. Compound assignment (+=)");
} else {
    console.log("✗ 12. Compound assignment FAILED");
}

// Test 13: Null and undefined
totalTests = totalTests + 1;
const nullVal = null;
let undefinedVal;
if (nullVal === null) {
    passedTests = passedTests + 1;
    console.log("✓ 13. Null value");
} else {
    console.log("✗ 13. Null value FAILED");
}

// Test 14: Object with methods
totalTests = totalTests + 1;
const obj = {
    value: 42,
    getValue: function() {
        return this.value;
    }
};
if (obj.getValue() === 42) {
    passedTests = passedTests + 1;
    console.log("✓ 14. Object methods");
} else {
    console.log("✗ 14. Object methods FAILED");
}

// Test 15: Class inheritance
totalTests = totalTests + 1;
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        return "sound";
    }
}
class Dog extends Animal {
    constructor(name) {
        super(name);
    }
    speak() {
        return "bark";
    }
}
const dog = new Dog("Rex");
if (dog.speak() === "bark") {
    passedTests = passedTests + 1;
    console.log("✓ 15. Class inheritance");
} else {
    console.log("✗ 15. Class inheritance FAILED");
}

console.log("\n=== RESULTS ===");
console.log("Total tests:", totalTests);
console.log("Passed:", passedTests);
console.log("Failed:", totalTests - passedTests);
console.log("Success rate:", Math.floor(passedTests * 100 / totalTests), "%");
