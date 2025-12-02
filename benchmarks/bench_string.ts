// String Operations Benchmark
const iterations = 100000;

// String concatenation
const concatStart = Date.now();
let str = "";
for (let i = 0; i < iterations; i++) {
    str += "a";
}
const concatTime = Date.now() - concatStart;

// Template literals
const templateStart = Date.now();
let str2 = "";
for (let i = 0; i < iterations; i++) {
    str2 = `${str2}a`;
}
const templateTime = Date.now() - templateStart;

// String methods
const testStr = "Hello World ".repeat(1000);
const methodsStart = Date.now();
for (let i = 0; i < 10000; i++) {
    testStr.toUpperCase();
    testStr.toLowerCase();
    testStr.split(" ");
    testStr.replace("World", "Nova");
    testStr.indexOf("World");
}
const methodsTime = Date.now() - methodsStart;

console.log(`Concat x${iterations}: ${concatTime}ms`);
console.log(`Template x${iterations}: ${templateTime}ms`);
console.log(`String methods x10000: ${methodsTime}ms`);
console.log(`Total: ${concatTime + templateTime + methodsTime}ms`);
