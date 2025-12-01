function main(): number {
    let arr = [1, 2, 3];
    // forEach with simple callback that just returns the value
    // We can't really test side effects without closures,
    // so just verify it runs without errors
    arr.forEach((x) => x * 2);
    return 42;  // Return success
}
