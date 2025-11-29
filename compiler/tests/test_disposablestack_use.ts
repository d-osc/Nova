// Test DisposableStack.use() with callback

function main(): number {
    let stack = new DisposableStack();
    console.log("Created DisposableStack");

    // Use with value and callback
    let resource = 100;
    stack.use(resource, () => {
        console.log("Resource callback called!");
    });
    console.log("Added resource via use()");

    console.log("Calling dispose...");
    stack.dispose();

    console.log("Done!");
    return 0;
}
