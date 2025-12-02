// Memory Usage Benchmark
const formatBytes = (bytes) => {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
};

// Initial memory
const initial = process.memoryUsage();
console.log('Initial Memory:');
console.log(`  Heap Used: ${formatBytes(initial.heapUsed)}`);
console.log(`  Heap Total: ${formatBytes(initial.heapTotal)}`);
console.log(`  RSS: ${formatBytes(initial.rss)}`);

// Allocate arrays
const arrays = [];
for (let i = 0; i < 100; i++) {
    arrays.push(new Array(100000).fill(i));
}

const afterArrays = process.memoryUsage();
console.log('\nAfter 100 arrays (100K each):');
console.log(`  Heap Used: ${formatBytes(afterArrays.heapUsed)}`);
console.log(`  Heap Total: ${formatBytes(afterArrays.heapTotal)}`);
console.log(`  RSS: ${formatBytes(afterArrays.rss)}`);

// Allocate objects
const objects = [];
for (let i = 0; i < 100000; i++) {
    objects.push({ id: i, name: `Object ${i}`, data: [1, 2, 3, 4, 5] });
}

const afterObjects = process.memoryUsage();
console.log('\nAfter 100K objects:');
console.log(`  Heap Used: ${formatBytes(afterObjects.heapUsed)}`);
console.log(`  Heap Total: ${formatBytes(afterObjects.heapTotal)}`);
console.log(`  RSS: ${formatBytes(afterObjects.rss)}`);

// String allocation
let bigString = '';
for (let i = 0; i < 100000; i++) {
    bigString += 'Hello World ';
}

const afterStrings = process.memoryUsage();
console.log('\nAfter large string:');
console.log(`  Heap Used: ${formatBytes(afterStrings.heapUsed)}`);
console.log(`  Heap Total: ${formatBytes(afterStrings.heapTotal)}`);
console.log(`  RSS: ${formatBytes(afterStrings.rss)}`);

console.log('\nMemory Growth:');
console.log(`  Heap Used: +${formatBytes(afterStrings.heapUsed - initial.heapUsed)}`);
console.log(`  RSS: +${formatBytes(afterStrings.rss - initial.rss)}`);
