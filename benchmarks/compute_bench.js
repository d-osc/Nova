// Compute Benchmark - Math operations
function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

function sumRange(start, end) {
    let sum = 0;
    let i = start;
    while (i <= end) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

const fact10 = factorial(10);
const sum100 = sumRange(1, 100);

console.log(fact10);
console.log(sum100);
