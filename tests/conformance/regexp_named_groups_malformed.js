// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
//
// Local regression gate for Annex B (non-unicode) named-group / back-reference
// compatibility fallbacks and the related `\xHH` string escape + `.test()`
// Boolean result. Mirrors the upstream Test262 case
//   annexB/built-ins/RegExp/named-groups/non-unicode-malformed.js
// without hard-coding that filename or expectation: this exercises the same
// semantic rules directly via return codes.

function main() {
    // \k<...> that is NOT a valid named back-reference is an IdentityEscape
    // (literal k) in non-unicode mode.
    if (!/\k<a>/.test("k<a>")) return 1;
    if (!/\k<4>/.test("k<4>")) return 2;
    if (!/\k<a/.test("k<a")) return 3;
    if (!/\k/.test("k")) return 4;

    // Named capture group with identity-escape body.
    if (!/(?<a>\a)/.test("a")) return 5;

    // Dangling decimal back-reference behaviour (Annex B non-unicode).
    // \1 with no group defined before it: LegacyOctalEscapeSequence (code 1).
    if (!/\k<a>\1/.test("k<a>\x01")) return 6;
    // \1 before its group is defined: unmatched reference = empty string.
    if (!/\1(b)\k<a>/.test("bk<a>")) return 7;
    // \k<a> interacting with a real numbered group (<a> is literal here).
    if (!/\k<a>(<a>x)/.test("k<a><a>x")) return 8;
    if (!/\1(b)\k<a>/.test("bk<a>")) return 9;

    // `.test()` must return a real Boolean so `=== true` holds.
    var t = /\k<a>/.test("k<a>");
    if (t !== true) return 10;
    var f = /\k<a>/.test("zzz");
    if (f !== false) return 11;

    // `\xHH` string escape decodes to a single byte.
    if ("k<a>\x01".length !== 5) return 12;
    if ("\x41" !== "A") return 13;
    if ("\x7F".charCodeAt(0) !== 127) return 14;

    return 0;
}
