const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];

console.log("Testing complex spread operator:");
console.log("arr2[0]:", arr2[0]); // Should be 1
console.log("arr2[1]:", arr2[1]); // Should be 2
console.log("arr2[2]:", arr2[2]); // Should be 3
console.log("arr2[3]:", arr2[3]); // Should be 4
console.log("arr2[4]:", arr2[4]); // Should be 5
