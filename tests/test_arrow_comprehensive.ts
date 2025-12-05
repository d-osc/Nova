// Test various arrow function scenarios

// 1. Arrow with expression body (implicit return)
const add = (a, b) => a + b;
console.log(add(10, 32)); // Should print 42

// 2. Arrow with block body and explicit return
const multiply = (a, b) => {
  return a * b;
};
console.log(multiply(6, 7)); // Should print 42

// 3. Arrow with block body and no return (implicit return undefined/void)
const doSomething = (x) => {
  const y = x + 1;
  // No return statement
};
doSomething(5); // Should not crash

// 4. Arrow function as callback
const numbers = [1, 2, 3];
// This would use forEach if available

console.log("All arrow function tests passed!");
