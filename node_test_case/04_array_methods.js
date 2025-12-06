// Test 04: Array Methods
// Tests array manipulation methods

console.log('=== Test 04: Array Methods ===');

// Array creation and length
const arr = [1, 2, 3, 4, 5];
console.log('Array length:', arr.length);

// push/pop
arr.push(6);
console.log('After push(6):', arr.length, 'elements');
const popped = arr.pop();
console.log('Popped:', popped, 'length now:', arr.length);

// shift/unshift
const shifted = arr.shift();
console.log('Shifted:', shifted, 'length now:', arr.length);
arr.unshift(1);
console.log('After unshift(1):', arr.length, 'elements');

// slice
const sliced = arr.slice(1, 3);
console.log('arr.slice(1, 3):', sliced.length, 'elements');

// concat
const arr2 = [6, 7, 8];
const combined = arr.concat(arr2);
console.log('arr.concat([6,7,8]):', combined.length, 'elements');

// indexOf
console.log('[1,2,3,4,5].indexOf(3):', [1, 2, 3, 4, 5].indexOf(3));

// join
const joined = [1, 2, 3].join('-');
console.log('[1,2,3].join("-"):', joined);

// reverse
const reversed = [1, 2, 3].reverse();
console.log('[1,2,3].reverse():', reversed);

// map
const doubled = [1, 2, 3].map(x => x * 2);
console.log('[1,2,3].map(x => x*2):', doubled);

// filter
const evens = [1, 2, 3, 4, 5, 6].filter(x => x % 2 === 0);
console.log('[1,2,3,4,5,6].filter(even):', evens);

// reduce
const sum = [1, 2, 3, 4, 5].reduce((acc, x) => acc + x, 0);
console.log('[1,2,3,4,5].reduce(sum):', sum);

// forEach
let forEachSum = 0;
[1, 2, 3].forEach(x => forEachSum += x);
console.log('[1,2,3].forEach(sum):', forEachSum);

// find
const found = [1, 2, 3, 4, 5].find(x => x > 3);
console.log('[1,2,3,4,5].find(x > 3):', found);

// some/every
const hasEven = [1, 2, 3].some(x => x % 2 === 0);
const allPositive = [1, 2, 3].every(x => x > 0);
console.log('[1,2,3].some(even):', hasEven);
console.log('[1,2,3].every(positive):', allPositive);

console.log('\n✓ Array Methods Test Complete');
