// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test RegExp and basic regex operations

function main(): number {
    // Basic regex test
    const re1 = /hello/;
    if (!re1.test("hello world")) return 1;
    if (re1.test("goodbye")) return 2;

    // Anchors
    if (!/^hello/.test("hello world")) return 3;
    if (!/world$/.test("hello world")) return 4;
    if (/^abc$/.test("xyz")) return 5;

    // Quantifiers
    if (!/a+/.test("aaa")) return 6;
    if (!/a?/.test("")) return 7;
    if (!/a{2,3}/.test("aa")) return 8;
    if (!/a{2,3}/.test("aaa")) return 9;

    // Character classes
    if (!/\d/.test("abc123")) return 10;
    if (!/\w/.test("hello")) return 11;
    if (!/\s/.test("hello world")) return 12;
    if (/[a-z]/.test("ABC")) return 13;
    if (!/[A-Z]/.test("ABC")) return 14;
    if (/[^a-z]/.test("abc")) return 15;  // negated class should not match any letter

    // Groups (capture)
    const m = "2024-06-15".match(/(\d{4})-(\d{2})-(\d{2})/);
    if (!m) return 16;
    if (m[0] !== "2024-06-15") return 17;
    if (m[1] !== "2024") return 18;
    if (m[2] !== "06") return 19;
    if (m[3] !== "15") return 20;

    // Alternation
    if (!/cat|dog/.test("I love cats")) return 21;
    if (!/cat|dog/.test("I love dogs")) return 22;

    // Flags: g (global)
    const g = "a1b2c3".replace(/\d/g, "X");
    if (g !== "aXbXcX") return 23;

    // Flags: i (case-insensitive)
    const i = "HELLO".replace(/hello/i, "world");
    if (i !== "world") return 24;

    // exec with capture groups
    const re = /(\w+)@(\w+)/;
    const result = re.exec("user@example");
    if (!result) return 25;
    if (result[1] !== "user") return 26;
    if (result[2] !== "example") return 27;

    // String.match with no match returns null
    if ("hello".match(/\d+/) !== null) return 28;

    // String.split with regex: "a1b2c3" split by /\d/ -> ["a","b","c",""]
    const parts = "a1b2c3".split(/\d/);
    if (parts.length !== 4) return 29;
    if (parts[0] !== "a") return 30;
    if (parts[1] !== "b") return 31;
    if (parts[2] !== "c") return 32;
    if (parts[3] !== "") return 33;

    // Capturing group via replace with $1 / $2
    const swapped = "John Smith".replace(/(\w+) (\w+)/, "$2 $1");
    if (swapped !== "Smith John") return 34;

    // Literal dot inside class
    if (!/[.]/.test(".")) return 35;

    // Constructor form (string -> RegExp)
    const re2 = new RegExp("\\d+");
    if (!re2.test("abc123")) return 36;

    return 0;
}
