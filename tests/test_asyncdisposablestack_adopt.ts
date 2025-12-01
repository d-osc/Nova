// Test AsyncDisposableStack.adopt() - add value with custom dispose callback

function main(): number {
    let stack = new AsyncDisposableStack();
    console.log("Created AsyncDisposableStack");

    // Create some "resource" (just a number for now)
    let resource = 42;

    // Adopt the resource with a custom dispose callback
    stack.adopt(resource, (value) => {
        console.log("Disposing resource with value:");
        console.log(value);
    });

    console.log("Adopted resource");
    console.log("Calling disposeAsync...");

    stack.disposeAsync();

    console.log("Dispose complete!");
    return 0;
}
