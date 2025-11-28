function main(): number {
    // decodeURI(string) - decodes a full URI (ES3)

    // Test with encoded spaces
    let encoded1 = "https://example.com/my%20path";
    let result1 = decodeURI(encoded1);
    console.log(result1);  // Should print: https://example.com/my path

    // Test with multiple encoded spaces
    let encoded2 = "https://example.com/path%20with%20spaces";
    let result2 = decodeURI(encoded2);
    console.log(result2);  // Should print: https://example.com/path with spaces

    // Test with URL that has no encoding (should pass through unchanged)
    let encoded3 = "https://example.com/path?query=value";
    let result3 = decodeURI(encoded3);
    console.log(result3);  // Should print: https://example.com/path?query=value

    // Return success code
    return 200;
}
