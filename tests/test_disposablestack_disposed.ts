// Test DisposableStack.disposed property

function main(): number {
    let stack = new DisposableStack();
    console.log("Created DisposableStack");

    // Check disposed before dispose
    if (stack.disposed) {
        console.log("ERROR: should not be disposed yet");
    } else {
        console.log("Correctly not disposed yet");
    }

    stack.dispose();
    console.log("Disposed!");

    // Check disposed after dispose
    if (stack.disposed) {
        console.log("Correctly disposed");
    } else {
        console.log("ERROR: should be disposed");
    }

    return 0;
}
