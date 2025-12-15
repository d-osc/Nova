console.log("Test 1: case 1");
const x = 1;
switch (x) {
    case 1:
        console.log("  Matched 1");
        break;
    case 2:
        console.log("  Matched 2");
        break;
}
console.log("  Done 1\n");

console.log("Test 2: case 2");
const y = 2;
switch (y) {
    case 1:
        console.log("  Matched 1");
        break;
    case 2:
        console.log("  Matched 2");
        break;
}
console.log("  Done 2\n");

console.log("Test 3: no match + default");
const z = 99;
switch (z) {
    case 1:
        console.log("  Matched 1");
        break;
    default:
        console.log("  Default case");
}
console.log("  Done 3");

console.log("\nAll tests complete!");
