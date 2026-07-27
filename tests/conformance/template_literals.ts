// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test template literals: interpolation, multiline, nested, tagged

function main(): number {
    const name = "world";
    const greeting = `hello ${name}`;
    if (greeting !== "hello world") return 1;

    // Multiple interpolations
    const a = 3, b = 4;
    const sum = `${a} + ${b} = ${a + b}`;
    if (sum !== "3 + 4 = 7") return 2;

    // Multiline
    const multi = `line1
line2
line3`;
    if (multi.length !== 17) return 3;  // 5+1+5+1+5
    if (multi.indexOf("line2") < 0) return 4;

    // Expression with method call
    const upper = `${"abc".toUpperCase()}`;
    if (upper !== "ABC") return 5;

    // Nested template
    const nested = `outer ${`inner ${1 + 1}`} end`;
    if (nested !== "outer inner 2 end") return 6;

    // Number formatting
    const num = 3.14;
    const pi = `pi=${num}`;
    if (pi !== "pi=3.14") return 7;

    // Boolean
    const bool = `${1 > 2}`;
    if (bool !== "false") return 8;

    // Array join via interpolation (toString)
    const arr = [1, 2, 3];
    const arrStr = `${arr}`;
    if (arrStr !== "1,2,3") return 9;

    // Object interpolation
    const obj = { x: 1 };
    const objStr = `${obj}`;
    if (objStr !== "[object Object]") return 10;

    // Empty template
    const empty = ``;
    if (empty !== "") return 11;

    // No interpolation
    const plain = `just text`;
    if (plain !== "just text") return 12;

    // Escape sequences
    const esc = `tab\there`;
    if (esc !== "tab\there" || esc.length !== 8) return 13;

    // Tagged template
    function tag(strings: string[], ...values: number[]): string {
        let result = strings[0];
        for (let i = 0; i < values.length; i++) {
            result += values[i] * 2;
            result += strings[i + 1];
        }
        return result;
    }
    const tagged = tag`a=${10}b=${20}c`;
    if (tagged !== "a=20b=40c") return 14;

    return 0;
}
