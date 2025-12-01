function main(): number {
    // btoa(string) - encodes a string to base64 (Web API)

    // Test with simple string
    let str1 = "Hello";
    let result1 = btoa(str1);
    console.log(result1);  // Should print: SGVsbG8=

    // Test with "Hello, World!"
    let str2 = "Hello, World!";
    let result2 = btoa(str2);
    console.log(result2);  // Should print: SGVsbG8sIFdvcmxkIQ==

    // Test with simple text
    let str3 = "abc";
    let result3 = btoa(str3);
    console.log(result3);  // Should print: YWJj

    // Return success code
    return 197;
}
