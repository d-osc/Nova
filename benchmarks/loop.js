// Loop benchmark - testing basic loop performance
let sum = 0;
const iterations = 100000000;

const start = Date.now();
for (let i = 0; i < iterations; i++) {
    sum += i;
}
const end = Date.now();

console.log(`Sum: ${sum}`);
console.log(`Time: ${end - start}ms`);
