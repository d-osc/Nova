function main(): number {
    let count: number = 0;
    function increment(): number {
        count = count + 1;
        return count;
    }
    increment();
    increment();
    console.log("count=" + count);
    return 0;
}
