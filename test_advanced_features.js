console.log("=== ADVANCED FEATURES ===");

// 1. Arrow functions
const add = (a, b) => a + b;
console.log("Arrow functions:", add(5, 3));

// 2. Spread operator
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];
console.log("Spread:", arr2.length);

// 3. Destructuring
const [x, y] = [10, 20];
console.log("Destructuring:", x, y);

// 4. Rest parameters
function sum(...nums) {
    let total = 0;
    for (let n of nums) {
        total = total + n;
    }
    return total;
}
console.log("Rest params:", sum(1, 2, 3, 4));

// 5. For-of loop
let count = 0;
for (let item of [1, 2, 3]) {
    count = count + item;
}
console.log("For-of:", count);

console.log("=== DONE ===");
