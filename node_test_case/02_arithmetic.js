// Test 02: Arithmetic Operations
// Tests mathematical operations

console.log('=== Test 02: Arithmetic Operations ===');

// Basic arithmetic
const a = 10;
const b = 3;

console.log('Addition:', a + b, '=', 13);
console.log('Subtraction:', a - b, '=', 7);
console.log('Multiplication:', a * b, '=', 30);
console.log('Division:', a / b, '=', 3.3333333333333335);
console.log('Modulo:', a % b, '=', 1);

// Increment/Decrement
let x = 5;
console.log('x++ (post):', x++, 'x is now:', x);
x = 5;
console.log('++x (pre):', ++x, 'x is now:', x);
x = 5;
console.log('x-- (post):', x--, 'x is now:', x);
x = 5;
console.log('--x (pre):', --x, 'x is now:', x);

// Compound assignment
let y = 10;
y += 5;
console.log('y += 5:', y);
y -= 3;
console.log('y -= 3:', y);
y *= 2;
console.log('y *= 2:', y);
y /= 4;
console.log('y /= 4:', y);

console.log('\n✓ Arithmetic Test Complete');
