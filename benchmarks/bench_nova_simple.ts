// Simple benchmark for Nova - avoid deep recursion
console.log("=== Nova Benchmark Suite ===");

// 1. Loop performance
const loopStart = Date.now();
let sum = 0;
for (let i = 0; i < 10000000; i++) {
    sum += i;
}
const loopTime = Date.now() - loopStart;
console.log(`Loop 10M iterations: ${loopTime}ms (sum=${sum})`);

// 2. Math operations
const mathStart = Date.now();
let result = 0;
for (let i = 0; i < 1000000; i++) {
    result = Math.sqrt(i) + Math.pow(i, 2) % 1000;
}
const mathTime = Date.now() - mathStart;
console.log(`Math 1M operations: ${mathTime}ms`);

// 3. Array operations
const arrStart = Date.now();
const arr: number[] = [];
for (let i = 0; i < 100000; i++) {
    arr.push(i);
}
const arrTime = Date.now() - arrStart;
console.log(`Array push 100K: ${arrTime}ms`);

// 4. String operations
const strStart = Date.now();
let str = "";
for (let i = 0; i < 10000; i++) {
    str = str + "a";
}
const strTime = Date.now() - strStart;
console.log(`String concat 10K: ${strTime}ms`);

// 5. Object creation
const objStart = Date.now();
const objects: any[] = [];
for (let i = 0; i < 100000; i++) {
    objects.push({ id: i, value: i * 2 });
}
const objTime = Date.now() - objStart;
console.log(`Object creation 100K: ${objTime}ms`);

// 6. Function calls
function add(a: number, b: number): number {
    return a + b;
}
const funcStart = Date.now();
let funcResult = 0;
for (let i = 0; i < 1000000; i++) {
    funcResult = add(funcResult, 1);
}
const funcTime = Date.now() - funcStart;
console.log(`Function calls 1M: ${funcTime}ms`);

// 7. Iterative fibonacci (no recursion)
function fibIterative(n: number): number {
    if (n <= 1) return n;
    let a = 0, b = 1;
    for (let i = 2; i <= n; i++) {
        const temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
const fibStart = Date.now();
const fibResult = fibIterative(40);
const fibTime = Date.now() - fibStart;
console.log(`Fibonacci(40) iterative: ${fibResult} in ${fibTime}ms`);

// 8. Prime counting (iterative)
function countPrimes(max: number): number {
    let count = 0;
    for (let n = 2; n <= max; n++) {
        let isPrime = true;
        for (let i = 2; i * i <= n; i++) {
            if (n % i === 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime) count++;
    }
    return count;
}
const primeStart = Date.now();
const primeCount = countPrimes(50000);
const primeTime = Date.now() - primeStart;
console.log(`Primes to 50000: ${primeCount} in ${primeTime}ms`);

// Total
const total = loopTime + mathTime + arrTime + strTime + objTime + funcTime + fibTime + primeTime;
console.log(`\nTotal: ${total}ms`);
