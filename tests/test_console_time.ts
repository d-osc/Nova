function main(): number {
    // console.time() - starts a timer with a label
    // console.timeEnd() - ends the timer and prints elapsed time
    // Used for performance measurement

    console.time("test");

    // Do some work
    let sum = 0;
    for (let i = 0; i < 100; i = i + 1) {
        sum = sum + i;
    }

    console.timeEnd("test");

    // Return success code
    return 170;
}
