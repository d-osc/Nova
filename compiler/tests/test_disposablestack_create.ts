// Test DisposableStack creation and dispose

function main(): number {
    // Create a DisposableStack
    let stack = new DisposableStack();
    console.log("Created DisposableStack");

    // Dispose (should do nothing with empty stack)
    stack.dispose();
    console.log("Stack disposed");

    console.log("All DisposableStack create tests passed!");
    return 0;
}
