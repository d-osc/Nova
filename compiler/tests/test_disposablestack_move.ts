// Test DisposableStack.move() - transfer ownership

function main(): number {
    let stack1 = new DisposableStack();
    console.log("Created stack1");

    stack1.defer(() => {
        console.log("Callback from original stack");
    });

    console.log("Added callback to stack1");

    // Move resources to a new stack
    let stack2 = stack1.move();
    console.log("Moved to stack2");

    // Dispose stack2 should execute the callback
    console.log("Disposing stack2...");
    stack2.dispose();

    console.log("Test passed!");
    return 0;
}
