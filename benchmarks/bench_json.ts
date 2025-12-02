// JSON Benchmark - Parse & Stringify
const iterations = 100000;

const testObject = {
    name: "Nova Benchmark",
    version: "1.0.0",
    features: ["fast", "efficient", "compatible"],
    nested: {
        level1: {
            level2: {
                level3: {
                    data: [1, 2, 3, 4, 5]
                }
            }
        }
    },
    numbers: Array.from({length: 100}, (_, i) => i * Math.random())
};

const jsonString = JSON.stringify(testObject);

// Stringify benchmark
const stringifyStart = Date.now();
for (let i = 0; i < iterations; i++) {
    JSON.stringify(testObject);
}
const stringifyTime = Date.now() - stringifyStart;

// Parse benchmark
const parseStart = Date.now();
for (let i = 0; i < iterations; i++) {
    JSON.parse(jsonString);
}
const parseTime = Date.now() - parseStart;

console.log(`JSON.stringify x${iterations}: ${stringifyTime}ms`);
console.log(`JSON.parse x${iterations}: ${parseTime}ms`);
console.log(`Total: ${stringifyTime + parseTime}ms`);
