function main(): number {
    // encodeURI(string) - encodes a full URI, preserving URI-valid characters (ES3)

    // Test with simple URL (should preserve : / ?)
    let url1 = "https://example.com/path?query=value";
    let result1 = encodeURI(url1);
    console.log(result1);  // Should print: https://example.com/path?query=value

    // Test with spaces (should encode spaces as %20)
    let url2 = "https://example.com/my path";
    let result2 = encodeURI(url2);
    console.log(result2);  // Should print: https://example.com/my%20path

    // Test with special characters that should be encoded
    let url3 = "https://example.com/path with spaces";
    let result3 = encodeURI(url3);
    console.log(result3);  // Should print: https://example.com/path%20with%20spaces

    // Return success code
    return 199;
}
