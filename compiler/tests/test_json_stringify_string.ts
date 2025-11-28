function main(): number {
    // JSON.stringify(string) - converts a string to a JSON string with quotes (ES5)

    // Test with simple string
    let str1 = "hello";
    let result1 = JSON.stringify(str1);
    console.log(result1);  // Should print: "hello"

    // Test with another string
    let str2 = "world";
    let result2 = JSON.stringify(str2);
    console.log(result2);  // Should print: "world"

    // Test with empty string
    let str3 = "";
    let result3 = JSON.stringify(str3);
    console.log(result3);  // Should print: ""

    // Return success code
    return 193;
}
