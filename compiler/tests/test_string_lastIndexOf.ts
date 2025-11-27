function main(): number {
    let str = "hello world hello";
    // Find last occurrence of "hello"
    let idx = str.lastIndexOf("hello");
    // Should return 12 (last "hello" starts at index 12)
    return idx;
}
