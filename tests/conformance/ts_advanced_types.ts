// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test advanced TS type features (type-only, no runtime impact)

type Point = { x: number; y: number };
type Keys = keyof Point;          // "x" | "y"
type Coord = string;              // simple alias
type Maybe<T> = T | null;         // generic conditional

// Conditional type — type-only
type IsNumber<T> = T extends number ? "yes" : "no";
type Result1 = IsNumber<number>;  // "yes"
type Result2 = IsNumber<string>;  // "no"

// keyof in type position
function getKey(): keyof Point {
    return "x";
}

// Generic with conditional return type
function classify<T>(v: T): T extends number ? "num" : "other" {
    return "" as any;
}

function main(): number {
    const p: Point = { x: 1, y: 2 };
    if (p.x !== 1) return 1;
    if (p.y !== 2) return 2;

    const k: Keys = "x";
    if (k !== "x") return 3;

    // Use of aliases
    const c: Coord = "hello";
    if (c !== "hello") return 4;

    // Maybe<T>
    const m: Maybe<number> = 42;
    if (m !== 42) return 5;

    return 0;
}
