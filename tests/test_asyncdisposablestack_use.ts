// Test AsyncDisposableStack.use() with callback
// Note: Full object disposal requires Symbol.asyncDispose support

function main(): number {
    let stack = new AsyncDisposableStack();
    console.log("Created AsyncDisposableStack");

    // Use with value and callback
    let resource = 100;
    stack.use(resource, () => {
        console.log("Resource callback called!");
    });
    console.log("Added resource via use()");

    console.log("Calling disposeAsync...");
    stack.disposeAsync();

    console.log("Done!");
    return 0;
}
