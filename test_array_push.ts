// Test optimized push operation
console.log("Testing Array push optimization");

let arr = [];
console.log("Initial length: 0");

arr.push(10);
arr.push(20);
arr.push(30);

console.log("After 3 pushes:");
console.log("Element 0:");
console.log(arr[0]);
console.log("Element 1:");
console.log(arr[1]);
console.log("Element 2:");
console.log(arr[2]);

console.log("indexOf test:");
let idx = arr.indexOf(20);
console.log(idx);
