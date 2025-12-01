function main(): number {
    // Date.now() - returns current timestamp in milliseconds since Unix epoch (ES5)

    // Get current time
    let timestamp = Date.now();

    // Verify timestamp is a reasonable value (greater than 0)
    // Timestamp should be in milliseconds since Jan 1, 1970
    // A timestamp from 2024 should be > 1700000000000

    if (timestamp > 1000000000000) {
        console.log(1);  // Should print 1 (timestamp is reasonable)
    } else {
        console.log(0);  // Would print 0 if something is wrong
    }

    // Call again to verify it returns consistent values
    let timestamp2 = Date.now();

    // Second timestamp should be >= first (time doesn't go backwards)
    if (timestamp2 >= timestamp) {
        console.log(1);  // Should print 1
    } else {
        console.log(0);
    }

    // Return success code
    return 188;
}
