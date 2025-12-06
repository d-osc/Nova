// Test 06: Math Operations
// Tests Math object methods

console.log('=== Test 06: Math Operations ===');

// Math constants
console.log('Math.PI:', Math.PI);
console.log('Math.E:', Math.E);

// Rounding
console.log('Math.round(4.7):', Math.round(4.7));
console.log('Math.round(4.4):', Math.round(4.4));
console.log('Math.ceil(4.1):', Math.ceil(4.1));
console.log('Math.floor(4.9):', Math.floor(4.9));

// Min/Max
console.log('Math.min(5, 10, 2, 8):', Math.min(5, 10, 2, 8));
console.log('Math.max(5, 10, 2, 8):', Math.max(5, 10, 2, 8));

// Absolute value
console.log('Math.abs(-42):', Math.abs(-42));
console.log('Math.abs(42):', Math.abs(42));

// Power and square root
console.log('Math.pow(2, 3):', Math.pow(2, 3));
console.log('Math.sqrt(16):', Math.sqrt(16));
console.log('Math.sqrt(2):', Math.sqrt(2));

// Random
const rand1 = Math.random();
const rand2 = Math.random();
console.log('Math.random() generates:', typeof rand1);
console.log('Math.random() in range:', rand1 >= 0 && rand1 < 1);

// Trigonometry
console.log('Math.sin(0):', Math.sin(0));
console.log('Math.cos(0):', Math.cos(0));
console.log('Math.tan(0):', Math.tan(0));

// Logarithm
console.log('Math.log(Math.E):', Math.log(Math.E));
console.log('Math.log10(100):', Math.log10(100));

console.log('\n✓ Math Operations Test Complete');
