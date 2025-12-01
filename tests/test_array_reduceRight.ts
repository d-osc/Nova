function main(): number {
    let arr = [1, 2, 3, 4];
    // reduceRight: processes from right to left
    // Build number: 0*10+4 = 4, 4*10+3 = 43, 43*10+2 = 432, 432*10+1 = 4321
    let result = arr.reduceRight((acc, x) => acc * 10 + x, 0);
    // Should return 4321 (right-to-left: 4,3,2,1)
    // reduce would give 1234 (left-to-right: 1,2,3,4)
    return result;
}
