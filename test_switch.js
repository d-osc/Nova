// Switch statement test
const x = 2;
let result = "";

switch (x) {
    case 1:
        result = "one";
        break;
    case 2:
        result = "two";
        break;
    case 3:
        result = "three";
        break;
    default:
        result = "other";
}

console.log("result:", result);
console.log("Expected: result: two");
