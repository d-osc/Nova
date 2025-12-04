// Test JSON.parse with different inputs

// Parse a number
const jsonNum = "42";
const num = JSON.parse(jsonNum);
console.log(num);

// Parse a string
const jsonStr = '"Hello"';
const str = JSON.parse(jsonStr);
console.log(str);

// Parse a boolean
const jsonBool = "true";
const flag = JSON.parse(jsonBool);
console.log(flag);
