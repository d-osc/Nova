function processUser() {
    // console.trace() - prints stack trace showing function call chain
    console.trace("User processing trace");
}

function handleRequest() {
    console.log("Handling request...");
    processUser();
}

function main(): number {
    // console.trace() with optional message
    // Shows where the trace was called from

    console.log("Starting trace test");

    // Direct trace call
    console.trace("Direct trace from main");

    // Trace through function calls
    handleRequest();

    console.log("Trace test complete");

    // Return success code
    return 183;
}
