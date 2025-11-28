function main(): number {
    // String.prototype.match(substring) - finds matches in string
    // Simplified implementation: returns count of matches

    let text = "hello world hello";

    // Match existing substring
    let count1 = text.match("hello");
    console.log(count1);  // Should print 2 (two occurrences)

    // Match single character
    let count2 = text.match("o");
    console.log(count2);  // Should print 3 (three occurrences)

    // Match non-existing substring
    let count3 = text.match("xyz");
    console.log(count3);  // Should print 0 (no occurrences)

    // Match space
    let count4 = text.match(" ");
    console.log(count4);  // Should print 2 (two spaces)

    // Return success code
    return 185;
}
