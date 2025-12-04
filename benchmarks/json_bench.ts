// JSON Benchmark - Parse and Stringify
const jsonData = {
    name: "Nova",
    version: "1.0.0",
    features: ["fast", "native", "typescript"],
    count: 42,
    active: true
};

const jsonString = JSON.stringify(jsonData);
const parsed = JSON.parse(jsonString);

console.log(parsed.name);
console.log(parsed.count);
