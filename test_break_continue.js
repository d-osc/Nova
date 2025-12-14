// Break and continue test
let sum = 0;

for (let i = 0; i < 10; i = i + 1) {
    if (i == 5) {
        continue;
    }
    if (i == 8) {
        break;
    }
    sum = sum + i;
}

console.log("sum:", sum);
console.log("Expected: sum: 28");
// 0+1+2+3+4+6+7 = 28 (skips 5, stops before 8)
