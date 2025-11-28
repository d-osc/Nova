function main(): number {
    // console.count(label) - increments and prints counter for label
    // console.countReset(label) - resets counter to zero
    // Used for tracking code execution frequency

    // Count with label "loop"
    console.count("loop");  // loop: 1
    console.count("loop");  // loop: 2
    console.count("loop");  // loop: 3

    // Count with different label
    console.count("test");  // test: 1
    console.count("test");  // test: 2

    // Reset and count again
    console.countReset("loop");
    console.count("loop");  // loop: 1

    // Return success code
    return 180;
}
