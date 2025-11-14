// String concatenation test - check if it compiles and assigns
function testStringConcat(): number {
    let s1 = "Hello";
    let s2 = " World";
    let s3 = s1 + s2;  // Should concatenate to "Hello World"
    
    // We can't check string content yet, just return success
    // In the future, we can check s3.length == 11
    return 100;
}

function main(): number {
    return testStringConcat();
}
