// Test Promise.then()

function main(): number {
    console.log("Creating Promise...");

    let p = new Promise();
    console.log("Promise created");

    // Add a then handler
    let p2 = p.then((value: number): number => {
        console.log("Then callback called!");
        return value * 2;
    });

    console.log("Then handler added");
    console.log("Test passed!");
    return 0;
}
