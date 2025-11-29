// Test true yield suspension

function* countUp(): number {
    console.log("Before first yield");
    yield 10;
    console.log("After first yield, before second");
    yield 20;
    console.log("After second yield, before third");
    yield 30;
    console.log("After third yield");
    return 99;
}

function main(): number {
    console.log("=== Testing yield suspension ===");

    let gen = countUp();
    console.log("Generator created");

    // First next() - should print "Before first yield" then yield 10
    console.log("Calling first next()...");
    let r1 = gen.next(0);
    console.log("First next() returned");

    // Check if done
    let done1 = r1.done;
    if (done1 == 0) {
        console.log("PASS: First yield - not done yet");
    } else {
        console.log("FAIL: First yield marked as done");
    }

    // Get value
    let val1 = r1.value;
    if (val1 == 10) {
        console.log("PASS: First yield value is 10");
    } else {
        console.log("FAIL: First yield value wrong");
    }

    console.log("Test completed!");
    return 0;
}
