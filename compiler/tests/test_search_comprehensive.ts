// Comprehensive test for includes() and indexOf()
function main(): number {
    let arr = [100, 200, 300, 400, 500];

    // Test includes() - found and not found
    let has200 = arr.includes(200);    // 1 (true)
    let has999 = arr.includes(999);    // 0 (false)
    let has100 = arr.includes(100);    // 1 (true)
    let has500 = arr.includes(500);    // 1 (true)

    // Test indexOf() - various positions and not found
    let idx100 = arr.indexOf(100);     // 0 (first element)
    let idx300 = arr.indexOf(300);     // 2 (middle)
    let idx500 = arr.indexOf(500);     // 4 (last element)
    let idx777 = arr.indexOf(777);     // -1 (not found)

    // Complex expression combining both methods
    // has200 (1) + has999 (0) + has100 (1) + has500 (1) = 3
    // idx100 (0) + idx300 (2) + idx500 (4) + idx777 (-1) = 5
    // Total: 3 + 5 = 8
    let includesSum = has200 + has999 + has100 + has500;
    let indexSum = idx100 + idx300 + idx500 + idx777;

    return includesSum + indexSum;  // Expected: 8
}
