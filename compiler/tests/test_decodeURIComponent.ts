function main(): number {
    // decodeURIComponent(string) - decodes a URI component (ES3)
    // Decodes percent-encoded characters back to original

    // Test with simple string (no decoding needed)
    let str1 = "hello";
    let result1 = decodeURIComponent(str1);
    console.log(result1);  // Should print: hello

    // Test with encoded space (%20)
    let str2 = "hello%20world";
    let result2 = decodeURIComponent(str2);
    console.log(result2);  // Should print: hello world

    // Test with encoded special characters
    let str3 = "a%3Db%26c%3Dd";
    let result3 = decodeURIComponent(str3);
    console.log(result3);  // Should print: a=b&c=d

    // Return success code
    return 196;
}
