// Comprehensive string operations test
function getLength(s: string): number {
    return s.length;  // Should call strlen at runtime
}

function main(): number {
    // Test 1: Direct string length (compile time)
    let len1 = "Hello".length;  // Should be 5

    // Test 2: String parameter length (runtime)
    let len2 = getLength("World");  // Should be 5

    // Test 3: String concatenation
    let s1 = "Hello";
    let s2 = " World";
    let s3 = s1 + s2;  // Should create "Hello World"

    // Test 4: Concatenated string length
    let len3 = getLength(s3);  // Should be 11

    // Return total: 5 + 5 + 11 = 21
    return len1 + len2 + len3;
}
