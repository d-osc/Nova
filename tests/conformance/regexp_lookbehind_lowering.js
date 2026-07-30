// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
//
// Local regression gate for zero-width lookbehind assertions
// ((?<=literal) / (?<!literal)), which std::regex does not support. Nova
// extracts literal lookbehinds into post-match constraints. Mirrors the
// upstream Test262 case
//   annexB/built-ins/RegExp/named-groups/non-unicode-malformed-lookbehind.js
// without hard-coding that filename or expectation.

function main() {
    // Positive zero-width lookbehind, single literal char (matches).
    if (!/\k<a>(?<=>)a/.test("k<a>a")) return 1;
    // Lookbehind at the start of the pattern (matches).
    if (!/(?<=>)\k<a>/.test(">k<a>")) return 2;
    // Negative zero-width lookbehind (matches when the preceding char differs).
    if (!/\k<a>(?<!a)a/.test("k<a>a")) return 3;
    if (!/(?<!a>)\k<a>/.test("k<a>")) return 4;
    // Negative lookbehind that must NOT match when the preceding char equals
    // the asserted value.
    if (/(?<!z)a/.test("za")) return 5;

    return 0;
}
