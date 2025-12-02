// Benchmark 4: Unrolled Fibonacci (no recursion/loops)
function fibonacci20(): number {
    const f0 = 0;
    const f1 = 1;
    const f2 = f0 + f1;
    const f3 = f1 + f2;
    const f4 = f2 + f3;
    const f5 = f3 + f4;
    const f6 = f4 + f5;
    const f7 = f5 + f6;
    const f8 = f6 + f7;
    const f9 = f7 + f8;
    const f10 = f8 + f9;
    const f11 = f9 + f10;
    const f12 = f10 + f11;
    const f13 = f11 + f12;
    const f14 = f12 + f13;
    const f15 = f13 + f14;
    const f16 = f14 + f15;
    const f17 = f15 + f16;
    const f18 = f16 + f17;
    const f19 = f17 + f18;
    const f20 = f18 + f19;
    return f20;
}

const result = fibonacci20();
console.log(result);
