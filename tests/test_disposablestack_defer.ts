// Test DisposableStack.defer() with arrow functions

function main(): number {
    let stack = new DisposableStack();
    console.log("Created DisposableStack");

    // Add defer callbacks that just print
    stack.defer(() => {
        console.log("Callback 1 executed");
    });

    stack.defer(() => {
        console.log("Callback 2 executed");
    });

    console.log("Added 2 defer callbacks");
    console.log("Calling dispose...");

    // Dispose should call callbacks in LIFO order
    // Should print: Callback 2 executed, then Callback 1 executed
    stack.dispose();

    console.log("Dispose complete!");
    return 0;
}
