// Test case: Large number of functions
function fn1(): number { return 1; }
function fn2(): number { return 2; }
function fn3(): number { return 3; }
function fn4(): number { return 4; }
function fn5(): number { return 5; }

function sum5(): number {
    return fn1() + fn2() + fn3() + fn4() + fn5();
}

function fn6(): number { return 6; }
function fn7(): number { return 7; }
function fn8(): number { return 8; }
function fn9(): number { return 9; }
function fn10(): number { return 10; }

function sum10(): number {
    return sum5() + fn6() + fn7() + fn8() + fn9() + fn10();
}
