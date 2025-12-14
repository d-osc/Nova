// Reality check - test various features

// 1. Template literals
const name = "Nova";
const greeting = `Hello ${name}!`;
console.log(greeting);

// 2. Spread operator
const arr1 = [1, 2];
const arr2 = [...arr1, 3];
console.log("Spread:", arr2);

// 3. Destructuring
const [x, y] = [10, 20];
console.log("Destructure:", x, y);

// 4. For-of loop
for (const item of [1, 2, 3]) {
    console.log("Item:", item);
}
