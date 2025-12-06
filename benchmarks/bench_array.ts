// Array Operations Benchmark
const size = 1000000;

// Create array
const createStart = Date.now();
const arr = [];
for (let i = 0; i < size; i++) {
    arr.push(i);
}
const createTime = Date.now() - createStart;

// Map
const mapStart = Date.now();
const mapped = arr.map(x => x * 2);
const mapTime = Date.now() - mapStart;

// Filter
const filterStart = Date.now();
const filtered = arr.filter(x => x % 2 === 0);
const filterTime = Date.now() - filterStart;

// Reduce
const reduceStart = Date.now();
const sum = arr.reduce((a, b) => a + b, 0);
const reduceTime = Date.now() - reduceStart;

// Sort (copy first)
const sortArr = [...arr].reverse();
const sortStart = Date.now();
sortArr.sort((a, b) => a - b);
const sortTime = Date.now() - sortStart;

console.log(`Create ${size} elements: ${createTime}ms`);
console.log(`Map: ${mapTime}ms`);
console.log(`Filter: ${filterTime}ms`);
console.log(`Reduce: ${reduceTime}ms`);
console.log(`Sort: ${sortTime}ms`);
console.log(`Total: ${createTime + mapTime + filterTime + reduceTime + sortTime}ms`);
