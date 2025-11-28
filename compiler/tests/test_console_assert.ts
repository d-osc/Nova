function main(): number {
    // console.assert(condition, message) - prints error if condition is false
    // Used for debugging assertions

    // This should NOT print (assertion passes)
    console.assert(1 == 1, "This should not appear");

    // This should NOT print (truthy value)
    console.assert(42, "This should not appear either");

    // This SHOULD print (assertion fails)
    console.assert(0, "Assertion failed: zero is falsy");

    // This SHOULD print (false condition)
    console.assert(1 == 2, "Assertion failed: 1 does not equal 2");

    // Return success code
    return 175;
}
