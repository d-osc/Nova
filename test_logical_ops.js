// Logical operators test
const a = 10;
const b = 20;

const and_result = a > 5 && b > 15;
console.log("and_result:", and_result);

const or_result = a > 100 || b > 15;
console.log("or_result:", or_result);

const not_result = !(a > 100);
console.log("not_result:", not_result);

console.log("\nExpected:");
console.log("and_result: true");
console.log("or_result: true");
console.log("not_result: true");
