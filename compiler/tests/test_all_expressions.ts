// Comprehensive test of all expression types
function main(): number {
    // 1. Arithmetic operators
    let a = 5 + 3;      // 8
    let b = 10 - 4;     // 6
    let c = 3 * 4;      // 12
    let d = 20 / 4;     // 5
    let e = 17 % 5;     // 2

    // 2. Comparison operators
    let eq = (5 == 5);
    let neq = (5 != 3);
    let lt = (3 < 5);
    let gt = (5 > 3);
    let lte = (3 <= 5);
    let gte = (5 >= 3);

    // 3. Logical operators
    let and = (true && true);
    let or = (false || true);
    let not = !false;

    // 4. Bitwise operators
    let band = 5 & 3;    // 1
    let bor = 5 | 3;     // 7
    let bxor = 5 ^ 3;    // 6
    let bnot = ~0;       // -1
    let lsh = 2 << 2;    // 8
    let rsh = 8 >> 2;    // 2

    // 5. Assignment operators
    let x = 10;
    x += 5;   // 15
    x -= 3;   // 12
    x *= 2;   // 24
    x /= 4;   // 6

    // 6. Increment/Decrement
    let y = 5;
    y++;      // 6
    y--;      // 5
    ++y;      // 6
    --y;      // 5

    // 7. Ternary operator
    let tern = (5 > 3) ? 1 : 0;  // 1

    // 8. Unary operators
    let neg = -5;
    let pos = +5;

    // 9. Array literal
    let arr = [1, 2, 3];

    // 10. Object literal
    let obj = { value: 42, name: "test" };

    // 11. Arrow function
    let add = (a: number, b: number): number => a + b;
    let result = add(3, 4);  // 7

    // 12. Function expression
    let multiply = function(x: number, y: number): number {
        return x * y;
    };
    let product = multiply(3, 4);  // 12

    // 13. Class expression
    let MyClass = class {
        val: number;
        constructor(v: number) {
            this.val = v;
        }
    };
    let instance = new MyClass(100);

    // 14. Array destructuring
    let nums = [10, 20, 30];
    let [first, second, third] = nums;

    // 15. Member access
    let len = arr.length;

    // 16. Computed member access
    let elem = arr[0];

    // 17. Typeof
    let typ = typeof 42;

    // 18. Comma operator
    let comma = (1, 2, 3);  // 3

    // 19. Template literal
    let name = "World";
    let greeting = `Hello ${name}!`;

    // Verification
    if (a != 8) return 1;
    if (b != 6) return 2;
    if (c != 12) return 3;
    if (d != 5) return 4;
    if (e != 2) return 5;
    if (!eq) return 6;
    if (!neq) return 7;
    if (!lt) return 8;
    if (!gt) return 9;
    if (!and) return 10;
    if (!or) return 11;
    if (!not) return 12;
    if (band != 1) return 13;
    if (bor != 7) return 14;
    if (bxor != 6) return 15;
    if (lsh != 8) return 16;
    if (rsh != 2) return 17;
    if (x != 6) return 18;
    if (y != 5) return 19;
    if (tern != 1) return 20;
    if (neg != -5) return 21;
    if (result != 7) return 22;
    if (product != 12) return 23;
    if (instance.val != 100) return 24;
    if (first != 10) return 25;
    if (second != 20) return 26;
    if (third != 30) return 27;
    if (comma != 3) return 28;

    return 0;
}
