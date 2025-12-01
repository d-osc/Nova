// Test DisposableStack.adopt() - add value with custom dispose callback

function main(): number {
    let stack = new DisposableStack();
    console.log("Created DisposableStack");

    // Create some "resource" (just a number for now)
    let resource = 42;

    // Adopt the resource with a custom dispose callback
    // The callback receives the resource value
    stack.adopt(resource, (value) => {
        console.log("Disposing resource with value:");
        console.log(value);
    });

    console.log("Adopted resource");
    console.log("Calling dispose...");

    stack.dispose();

    console.log("Dispose complete!");
    return 0;
}
