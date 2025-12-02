// benchmarks/bundler_test/src/math.js
function add(a, b) {
  return a + b;
}
function subtract(a, b) {
  return a - b;
}
function multiply(a, b) {
  return a * b;
}
function divide(a, b) {
  return b !== 0 ? a / b : 0;
}

// benchmarks/bundler_test/src/greetings.js
function greet(name) {
  return `Hello, ${name}!`;
}
function farewell(name) {
  return `Goodbye, ${name}!`;
}

// benchmarks/bundler_test/src/date.js
function formatDate(date) {
  return date.toISOString().split("T")[0];
}

// benchmarks/bundler_test/src/validators.js
function validateEmail(email) {
  const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  return re.test(email);
}
function validatePhone(phone) {
  const re = /^\+?[\d\s-]{10,}$/;
  return re.test(phone);
}

// benchmarks/bundler_test/src/index.js
console.log("Math operations:");
console.log("5 + 3 =", add(5, 3));
console.log("10 - 4 =", subtract(10, 4));
console.log("6 * 7 =", multiply(6, 7));
console.log("20 / 5 =", divide(20, 5));
console.log(`
Greetings:`);
console.log(greet("World"));
console.log(farewell("World"));
console.log(`
Validation:`);
console.log("test@example.com:", validateEmail("test@example.com"));
console.log("+1234567890:", validatePhone("+1234567890"));
console.log(`
Date:`);
console.log("Today:", formatDate(new Date));
export {
  subtract,
  multiply,
  greet,
  farewell,
  divide,
  add
};
