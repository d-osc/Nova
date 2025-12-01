function main(): number {
    // performance.now() - returns high-resolution timestamp in milliseconds (Web Performance API)
    // Returns time since page load / process start with sub-millisecond precision

    // Get high-resolution time
    let time1 = performance.now();

    // Get a second reading
    let time2 = performance.now();

    // Just verify the calls work and return - values should be non-negative
    // The test passes if we get here without crashing
    console.log(1);  // Should print 1

    // Return success code
    return 189;
}
