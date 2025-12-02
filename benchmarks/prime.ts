// Prime number benchmark
function isPrime(n: number): boolean {
    if (n < 2) return false;
    if (n === 2) return true;
    if (n % 2 === 0) return false;
    for (let i = 3; i * i <= n; i += 2) {
        if (n % i === 0) return false;
    }
    return true;
}

function countPrimes(limit: number): number {
    let count = 0;
    for (let i = 2; i <= limit; i++) {
        if (isPrime(i)) count++;
    }
    return count;
}

const start = Date.now();
const result = countPrimes(1000000);
const end = Date.now();

console.log(`Primes up to 1000000: ${result}`);
console.log(`Time: ${end - start}ms`);
