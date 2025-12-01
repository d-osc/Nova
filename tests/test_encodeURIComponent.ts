function main(): number {
    // encodeURIComponent(string) - encodes a URI component (ES3)
    // Encodes special characters except: A-Z a-z 0-9 - _ . ! ~ * ' ( )

    // Test with simple string (no encoding needed)
    let str1 = "hello";
    let result1 = encodeURIComponent(str1);
    console.log(result1);  // Should print: hello

    // Test with space (gets encoded as %20)
    let str2 = "hello world";
    let result2 = encodeURIComponent(str2);
    console.log(result2);  // Should print: hello%20world

    // Test with special characters
    let str3 = "a=b&c=d";
    let result3 = encodeURIComponent(str3);
    console.log(result3);  // Should print: a%3Db%26c%3Dd

    // Return success code
    return 195;
}
