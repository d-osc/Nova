// Test AsyncDisposableStack.defer()

function main(): number {
    let stack = new AsyncDisposableStack();
    console.log("Created AsyncDisposableStack");

    stack.defer(() => {
        console.log("Async callback 1");
    });

    stack.defer(() => {
        console.log("Async callback 2");
    });

    console.log("Added callbacks");
    console.log("Calling disposeAsync...");

    stack.disposeAsync();

    console.log("Done!");
    return 0;
}
