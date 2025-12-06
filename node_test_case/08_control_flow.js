// Test 08: Control Flow
// Tests if/else, loops, switch

console.log('=== Test 08: Control Flow ===');

// If/Else
const x = 10;
if (x > 5) {
    console.log('x > 5: true');
} else {
    console.log('x > 5: false');
}

const y = 3;
if (y > 10) {
    console.log('y > 10');
} else if (y > 5) {
    console.log('5 < y <= 10');
} else {
    console.log('y <= 5: true');
}

// Ternary operator
const result = x > 5 ? 'greater' : 'smaller';
console.log('Ternary result:', result);

// For loop
console.log('For loop:');
for (let i = 0; i < 5; i++) {
    console.log('  i =', i);
}

// While loop
console.log('While loop:');
let count = 0;
while (count < 3) {
    console.log('  count =', count);
    count++;
}

// Do-while loop
console.log('Do-while loop:');
let n = 0;
do {
    console.log('  n =', n);
    n++;
} while (n < 2);

// For...of loop
console.log('For...of loop:');
const arr = [10, 20, 30];
for (const val of arr) {
    console.log('  value =', val);
}

// Switch statement
const day = 2;
let dayName;
switch (day) {
    case 1:
        dayName = 'Monday';
        break;
    case 2:
        dayName = 'Tuesday';
        break;
    case 3:
        dayName = 'Wednesday';
        break;
    default:
        dayName = 'Other';
}
console.log('Switch result:', dayName);

// Break and continue
console.log('Break example:');
for (let i = 0; i < 5; i++) {
    if (i === 3) break;
    console.log('  i =', i);
}

console.log('Continue example:');
for (let i = 0; i < 5; i++) {
    if (i === 2) continue;
    console.log('  i =', i);
}

console.log('\n✓ Control Flow Test Complete');
