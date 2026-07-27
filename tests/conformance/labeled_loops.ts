// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Labeled break - exit outer loop from inside inner
    let result = 0;
    outer1:
    for (let i = 0; i < 5; i++) {
        for (let j = 0; j < 5; j++) {
            if (i === 2 && j === 2) {
                break outer1;  // should break OUTER loop entirely
            }
            result++;
        }
    }
    // i=0,j=0..4 (5) + i=1,j=0..4 (5) + i=2,j=0..1 (2) = 12, then break outer
    if (result !== 12) return 1;

    // Labeled continue - skip iteration of OUTER loop
    let result2 = 0;
    outer2:
    for (let i = 0; i < 4; i++) {
        for (let j = 0; j < 4; j++) {
            if (j === 2) {
                continue outer2;  // skip rest of inner, advance OUTER
            }
            result2++;
        }
    }
    // For each i: j=0 (+1), j=1 (+1), j=2 continue outer, so 2 per i = 8 total
    if (result2 !== 8) return 2;

    // While loop with label
    let result3 = 0;
    let i = 0;
    outer3:
    while (i < 3) {
        let j = 0;
        while (j < 3) {
            if (i === 1 && j === 1) {
                break outer3;
            }
            result3++;
            j++;
        }
        i++;
    }
    // i=0: j=0,1,2 (3), then i=1: j=0 (1), then break outer
    if (result3 !== 4) return 3;

    return 0;
}
