// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test String prototype methods comprehensively

function main(): number {
    // slice / substring / substr
    if ("hello world".slice(0, 5) !== "hello") return 1;
    if ("hello world".slice(6) !== "world") return 2;
    if ("hello world".slice(-5) !== "world") return 3;
    if ("hello world".substring(0, 5) !== "hello") return 4;

    // substr (deprecated but supported)
    if ("hello world".substr(6, 5) !== "world") return 5;

    // Case conversion
    if ("Hello".toLowerCase() !== "hello") return 6;
    if ("Hello".toUpperCase() !== "HELLO") return 7;

    // trim family
    if ("  hi  ".trim() !== "hi") return 8;
    if ("  hi  ".trimStart() !== "hi  ") return 9;
    if ("  hi  ".trimEnd() !== "  hi") return 10;
    if ("  hi  ".trimLeft() !== "hi  ") return 11;
    if ("  hi  ".trimRight() !== "  hi") return 12;

    // padStart / padEnd
    if ("5".padStart(3, "0") !== "005") return 13;
    if ("5".padStart(2, "0") !== "05") return 14;
    if ("5".padEnd(3, "0") !== "500") return 15;

    // repeat
    if ("ab".repeat(3) !== "ababab") return 16;
    if ("x".repeat(0) !== "") return 17;

    // indexOf / lastIndexOf
    if ("hello".indexOf("l") !== 2) return 18;
    if ("hello".lastIndexOf("l") !== 3) return 19;
    if ("hello".indexOf("z") !== -1) return 20;

    // includes / startsWith / endsWith
    if (!"hello".includes("ell")) return 21;
    if ("hello".includes("xyz")) return 22;
    if (!"hello".startsWith("he")) return 23;
    if ("hello".startsWith("lo")) return 24;
    if (!"hello".endsWith("lo")) return 25;
    if ("hello".endsWith("he")) return 26;

    // charAt / charCodeAt
    if ("hello".charAt(0) !== "h") return 27;
    if ("hello".charAt(10) !== "") return 28;
    if ("hello".charCodeAt(0) !== 104) return 29;  // 'h'

    // split
    const words = "a,b,c".split(",");
    if (words.length !== 3 || words[0] !== "a" || words[2] !== "c") return 30;

    const chars = "abc".split("");
    if (chars.length !== 3 || chars[1] !== "b") return 31;

    if ("abc".split("").length !== 3) return 32;

    // replace / replaceAll
    if ("hello".replace("l", "L") !== "heLlo") return 33;
    if ("aaa".replaceAll("a", "b") !== "bbb") return 34;

    // concat
    if ("a".concat("b", "c") !== "abc") return 35;

    // at (negative indexing)
    if ("hello".at(-1) !== "o") return 36;
    if ("hello".at(0) !== "h") return 37;

    // length property
    if ("hello".length !== 5) return 38;
    if ("".length !== 0) return 39;

    // template strings via concatenation
    if (("a" + "b" + "c").length !== 3) return 40;

    // String.fromCharCode
    if (String.fromCharCode(72, 73) !== "HI") return 41;

    // Array-like access
    const s = "hello";
    if (s[0] !== "h" || s[4] !== "o") return 42;

    // Search with regex (basic)
    if ("hello 2024".search(/[0-9]+/) < 0) return 43;

    // Match with regex
    const m = "abc123".match(/[0-9]+/);
    if (!m || m[0] !== "123") return 44;

    return 0;
}
