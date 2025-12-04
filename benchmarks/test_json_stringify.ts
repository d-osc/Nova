// Test JSON.stringify with different types

// Test with number
const num = 42;
const jsonNum = JSON.stringify(num);
console.log(jsonNum);

// Test with string
const str = "Hello World";
const jsonStr = JSON.stringify(str);
console.log(jsonStr);

// Test with boolean
const flag = true;
const jsonBool = JSON.stringify(flag);
console.log(jsonBool);

// Test with array
const arr = [1, 2, 3];
const jsonArr = JSON.stringify(arr);
console.log(jsonArr);
