// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test array iteration methods comprehensively

function main(): number {
    const nums = [1, 2, 3, 4, 5];

    // map
    const doubled = nums.map((x: number) => x * 2);
    if (doubled.length !== 5) return 1;
    if (doubled[0] !== 2 || doubled[4] !== 10) return 2;

    // filter
    const evens = nums.filter((x: number) => x % 2 === 0);
    if (evens.length !== 2) return 3;
    if (evens[0] !== 2 || evens[1] !== 4) return 4;

    // reduce (sum)
    const sum = nums.reduce((acc: number, x: number) => acc + x, 0);
    if (sum !== 15) return 5;

    // reduce without initial value
    const sum2 = nums.reduce((acc: number, x: number) => acc + x);
    if (sum2 !== 15) return 6;

    // reduceRight
    const reversed = nums.reduceRight((acc: number[], x: number) => {
        acc.push(x);
        return acc;
    }, [] as number[]);
    if (reversed.length !== 5 || reversed[0] !== 5 || reversed[4] !== 1) return 7;

    // forEach (side effect)
    let count = 0;
    nums.forEach((x: number) => { count++; });
    if (count !== 5) return 8;

    // forEach with index
    let indexSum = 0;
    nums.forEach((x: number, i: number) => { indexSum += i; });
    if (indexSum !== 10) return 9;  // 0+1+2+3+4

    // find
    const found = nums.find((x: number) => x > 3);
    if (found !== 4) return 10;

    const notFound = nums.find((x: number) => x > 100);
    if (notFound !== undefined) return 11;

    // findIndex
    const idx = nums.findIndex((x: number) => x > 3);
    if (idx !== 3) return 12;

    const idxNotFound = nums.findIndex((x: number) => x > 100);
    if (idxNotFound !== -1) return 13;

    // some (true if any match)
    const hasEven = nums.some((x: number) => x % 2 === 0);
    if (!hasEven) return 14;

    const hasTen = nums.some((x: number) => x === 10);
    if (hasTen) return 15;

    // every (true if all match)
    const allPositive = nums.every((x: number) => x > 0);
    if (!allPositive) return 16;

    const allEven = nums.every((x: number) => x % 2 === 0);
    if (allEven) return 17;

    // flatMap
    const flattened = [1, 2, 3].flatMap((x: number) => [x, x * 10]);
    if (flattened.length !== 6) return 18;
    if (flattened[0] !== 1 || flattened[1] !== 10) return 19;
    if (flattened[5] !== 30) return 20;

    // flat
    const nested = [[1, 2], [3, 4], [5]];
    const flat = nested.flat();
    if (flat.length !== 5) return 21;
    if (flat[0] !== 1 || flat[4] !== 5) return 22;

    // includes
    if (!nums.includes(3)) return 23;
    if (nums.includes(10)) return 24;

    // indexOf / lastIndexOf
    const withDups = [1, 2, 3, 2, 1];
    if (withDups.indexOf(2) !== 1) return 25;
    if (withDups.lastIndexOf(2) !== 3) return 26;

    // sort numeric (default is lexicographic!)
    const unsorted = [3, 1, 4, 1, 5, 9, 2, 6];
    const sortedNum = unsorted.slice().sort((a: number, b: number) => a - b);
    if (sortedNum.length !== 8) return 27;
    if (sortedNum[0] !== 1 || sortedNum[7] !== 9) return 28;

    // sort descending
    const descNum = unsorted.slice().sort((a: number, b: number) => b - a);
    if (descNum[0] !== 9 || descNum[7] !== 1) return 29;

    // reverse
    const rev = [1, 2, 3].reverse();
    if (rev.length !== 3 || rev[0] !== 3 || rev[2] !== 1) return 30;

    // slice
    const sl = [1, 2, 3, 4, 5].slice(1, 4);
    if (sl.length !== 3 || sl[0] !== 2 || sl[2] !== 4) return 31;

    // join
    if ([1, 2, 3].join("-") !== "1-2-3") return 32;
    if (["a", "b", "c"].join("") !== "abc") return 33;

    // concat
    const cat = [1, 2].concat([3, 4]);
    if (cat.length !== 4 || cat[3] !== 4) return 34;

    // fill
    const filled = new Array<number>(3).fill(0);
    if (filled.length !== 3 || filled[0] !== 0 || filled[2] !== 0) return 35;

    // at (negative indexing)
    if ([1, 2, 3].at(-1) !== 3) return 36;
    if ([1, 2, 3].at(0) !== 1) return 37;

    return 0;
}
