// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test switch statement with C-style fallthrough (no break between cases)

function main(): number {
    // Fallthrough should accumulate counter
    let count = 0;
    switch (1) {
        case 1:
            count += 1;  // matches, count = 1
            // FALLTHROUGH (no break)
        case 2:
            count += 10;  // should run due to fallthrough, count = 11
            // FALLTHROUGH
        case 3:
            count += 100;  // should run due to fallthrough, count = 111
            break;
        case 4:
            count += 1000;  // should NOT run
            break;
    }
    if (count !== 111) return 1;

    // Break in the middle stops fallthrough
    let count2 = 0;
    switch ("b") {
        case "a":
            count2 += 1;
        case "b":
            count2 += 10;  // matches, count2 = 10
            break;          // stops here
        case "c":
            count2 += 100;  // should NOT run
    }
    if (count2 !== 10) return 2;

    // Default in the middle (with fallthrough into it)
    let count3 = 0;
    switch (99) {
        case 1:
            count3 += 1;
            break;
        default:
            count3 += 1000;  // default runs
            // fallthrough
        case 2:
            count3 += 10;   // should this run? In JS, NO - default doesn't fall through to next case unless explicit
            break;
    }
    // JS semantics: default is just another case; if no break, fallthrough applies
    // For value 99, default body runs, count3 = 1000, then fallthrough to case 2 → count3 = 1010
    if (count3 !== 1010) return 3;

    return 0;
}
