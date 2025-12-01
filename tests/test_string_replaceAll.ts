function main(): number {
    // String.prototype.replaceAll() - replace all occurrences
    // Unlike replace() which only replaces the first occurrence,
    // replaceAll() replaces ALL occurrences of the search string
    // ES2021 feature

    // Test 1: Replace all 'o' with 'a'
    // "hello world" -> "hella warld"
    let str1 = "hello world";
    let result1 = str1.replaceAll("o", "a");
    // result1.length should be 11 (same length)

    // Test 2: Replace all 'l' with 'L'
    // "hello world" -> "heLLo worLd"
    let str2 = "hello world";
    let result2 = str2.replaceAll("l", "L");
    // result2.length should be 11

    // Test 3: Replace all spaces with underscores
    // "hello world" -> "hello_world"
    let str3 = "hello world";
    let result3 = str3.replaceAll(" ", "_");
    // result3.length should be 11

    // Return sum of lengths: 11 + 11 + 11 = 33
    return result1.length + result2.length + result3.length;
}
