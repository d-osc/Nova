// Simple switch test with just numbers
console.log("Testing basic switch:");

const num = 2;
console.log("num =", num);

switch (num) {
    case 1:
        console.log("Case 1");
        break;
    case 2:
        console.log("Case 2 - SHOULD PRINT THIS");
        break;
    case 3:
        console.log("Case 3");
        break;
}

console.log("After switch");
console.log("Test complete!");
