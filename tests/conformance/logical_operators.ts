// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if ((0 || 7) != 7) return 1;
    if ((4 && 9) != 9) return 2;
    if ((0 && 9) != 0) return 3;
    if ((4 || 9) != 4) return 4;
    if (("" || "fallback") != "fallback") return 5;
    if (("left" && "right") != "right") return 6;

    let hits = 0;
    false && ((hits = 1) == 1);
    if (hits != 0) return 7;
    true || ((hits = 2) == 2);
    if (hits != 0) return 8;
    true && ((hits = 3) == 3);
    if (hits != 3) return 9;
    false || ((hits = 4) == 4);
    if (hits != 4) return 10;

    if ((null ?? 8) != 8) return 11;
    if ((undefined ?? 9) != 9) return 12;
    if ((0 ?? 10) != 0) return 13;
    if ((false ?? true) != false) return 14;
    if (("" ?? "fallback") != "") return 15;

    let orValue = 0;
    orValue ||= 7;
    if (orValue != 7) return 16;

    let andValue = 4;
    andValue &&= 9;
    if (andValue != 9) return 17;

    let empty = "";
    empty ||= "filled";
    if (empty != "filled") return 18;

    let nanValue = NaN;
    nanValue ||= 6.5;
    if (nanValue != 6.5) return 19;

    let negativeZero = -0.0;
    negativeZero ||= 1.0;
    if (negativeZero != 1.0) return 20;

    let assignmentHits = 0;
    let keep = 5;
    keep ||= (assignmentHits = 1);
    if (assignmentHits != 0 || keep != 5) return 21;

    let stop = 0;
    stop &&= (assignmentHits = 2);
    if (assignmentHits != 0 || stop != 0) return 22;

    return 0;
}
