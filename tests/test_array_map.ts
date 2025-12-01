function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let doubled = arr.map((x) => x * 2);
    // doubled should be [2, 4, 6, 8, 10]
    // Return first element to verify (should be 2)
    return doubled[0];
}
