// Test array operations only
console.log("Creating array...");
const arr = [];
for (let i = 0; i < 10; i++) {
    arr.push(i);
}

console.log("Array created with " + arr.length + " elements");

console.log("Mapping array...");
const mapped = arr.map(x => x * 2);

console.log("Mapped array has " + mapped.length + " elements");
console.log("First element: " + mapped[0]);
console.log("Last element: " + mapped[9]);
console.log("Done!");
