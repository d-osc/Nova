// Test Promise.reject()

function main(): number {
    console.log("Testing Promise.reject...");

    // Create a rejected promise
    let p = Promise.reject(404);
    console.log("Promise.reject(404) created");

    // Add a catch handler
    p.catch((error: number): number => {
        console.log("Catch callback received error");
        return 0;
    });

    console.log("Test passed!");
    return 0;
}
