// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "nested-closure:15"

function outer(a: number) {
    let b: number = 2;
    function middle(c: number) {
        let d: number = 4;
        return function(e: number): number {
            a = a + 1;
            d = d + 1;
            return a + b + c + d + e;
        };
    }
    return middle(3);
}

function main(): number {
    let nested = outer(1);
    let first: number = nested(3);
    console.log("nested-closure:" + first);
    return 0;
}
