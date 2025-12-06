// Test 03: String Methods
// Tests string manipulation methods

console.log('=== Test 03: String Methods ===');

const str = 'Hello World';

// Length
console.log('str.length:', str.length);

// charAt
console.log('str.charAt(0):', str.charAt(0));
console.log('str.charAt(6):', str.charAt(6));

// indexOf
console.log('str.indexOf("World"):', str.indexOf('World'));
console.log('str.indexOf("o"):', str.indexOf('o'));

// toLowerCase/toUpperCase
console.log('str.toLowerCase():', str.toLowerCase());
console.log('str.toUpperCase():', str.toUpperCase());

// substring
console.log('str.substring(0, 5):', str.substring(0, 5));
console.log('str.substring(6):', str.substring(6));

// split
const parts = 'a,b,c,d'.split(',');
console.log('"a,b,c,d".split(","):', parts.length, 'parts');

// concat
const greeting = 'Hello'.concat(' ', 'Nova');
console.log('"Hello".concat(" ", "Nova"):', greeting);

// trim
const padded = '  spaces  ';
console.log('"  spaces  ".trim():', padded.trim());

// Template literals
const name = 'Nova';
const version = 1;
console.log('Template literal:', `${name} v${version}`);

console.log('\n✓ String Methods Test Complete');
