// Test 10: Comparison and Logical Operators
// Tests ==, ===, !=, !==, <, >, <=, >=, &&, ||, !

console.log('=== Test 10: Comparison and Logical Operators ===');

// Equality
console.log('5 == 5:', 5 == 5);
console.log('5 == "5":', 5 == "5");
console.log('5 === 5:', 5 === 5);
console.log('5 === "5":', 5 === "5");

// Inequality
console.log('5 != 3:', 5 != 3);
console.log('5 != "5":', 5 != "5");
console.log('5 !== 5:', 5 !== 5);
console.log('5 !== "5":', 5 !== "5");

// Comparison
console.log('5 < 10:', 5 < 10);
console.log('5 > 10:', 5 > 10);
console.log('5 <= 5:', 5 <= 5);
console.log('10 >= 5:', 10 >= 5);

// Logical AND
console.log('true && true:', true && true);
console.log('true && false:', true && false);
console.log('false && true:', false && true);

// Logical OR
console.log('true || false:', true || false);
console.log('false || false:', false || false);
console.log('true || true:', true || true);

// Logical NOT
console.log('!true:', !true);
console.log('!false:', !false);

// Combined logic
const a = 5;
const b = 10;
const c = 15;
console.log('(a < b) && (b < c):', (a < b) && (b < c));
console.log('(a > b) || (b < c):', (a > b) || (b < c));
console.log('!(a > b):', !(a > b));

// Truthy and Falsy
console.log('Boolean(0):', Boolean(0));
console.log('Boolean(1):', Boolean(1));
console.log('Boolean(""):', Boolean(""));
console.log('Boolean("hello"):', Boolean("hello"));
console.log('Boolean(null):', Boolean(null));
console.log('Boolean(undefined):', Boolean(undefined));

console.log('\n✓ Comparison Operators Test Complete');
