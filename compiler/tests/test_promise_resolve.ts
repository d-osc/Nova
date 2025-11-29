// Test Promise.resolve()

function main(): number {
    console.log("Testing Promise.resolve...");

    // Create a resolved promise
    let p = Promise.resolve(42);
    console.log("Promise.resolve(42) created");

    // Add a then handler
    p.then((value: number): number => {
        console.log("Then callback received value");
        return value;
    });

    console.log("Test passed!");
    return 0;
}
