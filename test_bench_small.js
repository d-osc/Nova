// Small Array Benchmark
const size = 100;

console.log(`Testing with ${size} elements`);

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

console.log(`Create ${size} elements: ${createTime}ms`);
console.log(`Map: ${mapTime}ms`);
console.log(`Total: ${createTime + mapTime}ms`);
