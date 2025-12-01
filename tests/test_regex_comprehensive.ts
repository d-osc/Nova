// Comprehensive regex test
function main(): number {
    let passed = 0;
    let failed = 0;

    // Test 1: Basic pattern matching
    let r1 = /hello/;
    if (r1.test("hello world")) {
        console.log("Test 1 PASS: Basic pattern match");
        passed = passed + 1;
    } else {
        console.log("Test 1 FAIL: Basic pattern match");
        failed = failed + 1;
    }

    // Test 2: No match
    let r2 = /goodbye/;
    if (r2.test("hello world")) {
        console.log("Test 2 FAIL: Should not match");
        failed = failed + 1;
    } else {
        console.log("Test 2 PASS: Correctly no match");
        passed = passed + 1;
    }

    // Test 3: Pattern with digits
    let r3 = /[0-9]+/;
    if (r3.test("abc123def")) {
        console.log("Test 3 PASS: Digit pattern");
        passed = passed + 1;
    } else {
        console.log("Test 3 FAIL: Digit pattern");
        failed = failed + 1;
    }

    // Test 4: Case insensitive flag
    let r4 = /HELLO/i;
    if (r4.test("hello world")) {
        console.log("Test 4 PASS: Case insensitive");
        passed = passed + 1;
    } else {
        console.log("Test 4 FAIL: Case insensitive");
        failed = failed + 1;
    }

    console.log("Results:");
    console.log(passed);
    console.log(failed);

    return failed;
}
