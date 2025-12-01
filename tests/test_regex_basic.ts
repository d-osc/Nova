// Test basic regex literal and test() method
function main(): number {
    let regex = /hello/;
    let str = "hello world";

    if (regex.test(str)) {
        console.log("Match found!");
    } else {
        console.log("No match");
    }

    // Test with non-matching string
    let str2 = "goodbye world";
    if (regex.test(str2)) {
        console.log("Match found unexpectedly");
    } else {
        console.log("Correctly no match");
    }

    console.log("Test completed");
    return 0;
}
