function main(): number {
    // Array.prototype.flatMap(callback)
    // Maps each element using callback, then flattens result one level (ES2019)
    // Like map() followed by flat()
    // For simple transformations (no nested arrays), behaves like map()

    let arr = [1, 2, 3];
    // arr = [1, 2, 3]

    // Transform each element: x => x * 2
    let result = arr.flatMap((x: number) => x * 2);
    // result = [2, 4, 6]

    // Test: result[1] = 4
    return result[1];
}
