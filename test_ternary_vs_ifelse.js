// Compare ternary vs if-else with strings

console.log("=== Test 1: If-Else with strings ===");
let result1;
if (5 > 3) {
    result1 = "yes";
} else {
    result1 = "no";
}
console.log("If-else result:", result1);

console.log("=== Test 2: Ternary with strings ===");
const result2 = 5 > 3 ? "yes" : "no";
console.log("Ternary result:", result2);

console.log("=== Test 3: If-Else with numbers ===");
let result3;
if (5 > 3) {
    result3 = 100;
} else {
    result3 = 200;
}
console.log("If-else result:", result3);

console.log("=== Test 4: Ternary with numbers ===");
const result4 = 5 > 3 ? 100 : 200;
console.log("Ternary result:", result4);
