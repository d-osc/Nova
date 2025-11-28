function main(): number {
    // atob(string) - decodes a base64 encoded string (Web API)

    // Test with simple base64 (decodes to "Hello")
    let encoded1 = "SGVsbG8=";
    let result1 = atob(encoded1);
    console.log(result1);  // Should print: Hello

    // Test with "Hello, World!" encoded
    let encoded2 = "SGVsbG8sIFdvcmxkIQ==";
    let result2 = atob(encoded2);
    console.log(result2);  // Should print: Hello, World!

    // Test with "abc" encoded
    let encoded3 = "YWJj";
    let result3 = atob(encoded3);
    console.log(result3);  // Should print: abc

    // Return success code
    return 198;
}
