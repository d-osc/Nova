// Test AsyncDisposableStack.disposed property

function main(): number {
    let stack = new AsyncDisposableStack();
    console.log("Created AsyncDisposableStack");

    // Check disposed before dispose
    if (stack.disposed) {
        console.log("ERROR: should not be disposed yet");
    } else {
        console.log("Correctly not disposed yet");
    }

    stack.disposeAsync();
    console.log("Disposed!");

    // Check disposed after dispose
    if (stack.disposed) {
        console.log("Correctly disposed");
    } else {
        console.log("ERROR: should be disposed");
    }

    return 0;
}
