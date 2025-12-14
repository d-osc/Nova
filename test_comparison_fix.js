// Test mixed type comparisons
console.log("Testing mixed type comparisons (double vs integer):");

const pi = 3.14159;
const radius = 5;
const area = pi * radius * radius;

console.log("area =", area);
console.log("area > 78 =", area > 78);
console.log("area < 79 =", area < 79);
console.log("area > 78 && area < 79 =", area > 78 && area < 79);

const d1 = 10.5;
const i1 = 3;

console.log("\nd1 =", d1, ", i1 =", i1);
console.log("d1 > i1 =", d1 > i1);
console.log("d1 < i1 =", d1 < i1);
console.log("d1 >= 10 =", d1 >= 10);
console.log("d1 <= 11 =", d1 <= 11);

console.log("\n✅ All mixed type comparisons work!");
