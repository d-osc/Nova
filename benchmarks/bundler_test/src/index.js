// Main entry point for bundler benchmark
import { add, subtract, multiply, divide } from './math.js';
import { greet, farewell } from './greetings.js';
import { fetchData, postData } from './api.js';
import { formatDate, parseDate } from './date.js';
import { validateEmail, validatePhone } from './validators.js';

console.log('Math operations:');
console.log('5 + 3 =', add(5, 3));
console.log('10 - 4 =', subtract(10, 4));
console.log('6 * 7 =', multiply(6, 7));
console.log('20 / 5 =', divide(20, 5));

console.log('\nGreetings:');
console.log(greet('World'));
console.log(farewell('World'));

console.log('\nValidation:');
console.log('test@example.com:', validateEmail('test@example.com'));
console.log('+1234567890:', validatePhone('+1234567890'));

console.log('\nDate:');
console.log('Today:', formatDate(new Date()));

export { add, subtract, multiply, divide, greet, farewell };
