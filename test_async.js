async function test() {
    console.log("Inside async function");
    const result = 42;
    return result;
}

console.log("Calling async function");
const promise = test();
console.log("Promise:", typeof promise);
console.log("Done");
