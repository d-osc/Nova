const arr1 = [1, 2];
const mixed = [0, ...arr1, 3];
console.log("mixed[0]:", mixed[0]);
console.log("mixed[1]:", mixed[1]);
console.log("mixed[2]:", mixed[2]);
console.log("mixed[3]:", mixed[3]);
console.log("\nExpected: 0, 1, 2, 3");
