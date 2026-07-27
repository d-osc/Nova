// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    if (Number("") !== 0) return 1;
    if (Number("  42  ") !== 42) return 2;
    if (!Number.isNaN(Number("not-a-number"))) return 3;
    if (String(null) !== "null") return 4;
    if (String(undefined) !== "undefined") return 5;
    if (Boolean(0) !== false || Boolean(-0) !== false) return 6;
    if (Boolean("") !== false || Boolean("0") !== true) return 7;

    if (null == undefined !== true) return 8;
    if (null === undefined) return 9;
    if ("1" == 1 !== true || "1" === 1) return 10;
    if (false == 0 !== true || false === 0) return 11;
    if (!(NaN !== NaN)) return 12;
    if (!Object.is(NaN, NaN)) return 13;
    if (Object.is(0, -0)) return 14;

    if (1 / 0 !== Infinity) return 15;
    if (1 / -0 !== -Infinity) return 16;
    if (Math.trunc(-1.9) !== -1) return 17;
    if ((2 ** 53) + 1 !== 2 ** 53) return 18;

    const preferredNumber = {
        valueOf() { return 7; },
        toString() { return "ignored"; }
    };
    if (preferredNumber + 1 !== 8) return 19;

    const preferredString = {
        toString() { return "nova"; },
        valueOf() { return 9; }
    };
    if (String(preferredString) !== "nova") return 20;

    return 0;
}
