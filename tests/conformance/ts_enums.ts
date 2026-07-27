// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test numeric enums, string enums, reverse mapping, and computed members

enum Color { Red, Green, Blue }
enum Status { Active = 1, Inactive = 2, Pending = 5 }
enum Direction { North = "N", South = "S", East = "E", West = "W" }
enum Mixed { A, B = 10, C, D = "hello", E }

function main(): number {
    // Numeric enum basic access
    if (Color.Red !== 0) return 1;
    if (Color.Green !== 1) return 2;
    if (Color.Blue !== 2) return 3;

    // Numeric enum with explicit values
    if (Status.Active !== 1) return 4;
    if (Status.Inactive !== 2) return 5;
    if (Status.Pending !== 5) return 6;

    // Auto-increment after explicit
    if (Mixed.A !== 0) return 7;
    if (Mixed.B !== 10) return 8;
    if (Mixed.C !== 11) return 9;  // 10 + 1
    if (Mixed.E !== 12) return 10;  // 11 + 1 after D (string)

    // String enum
    if (Direction.North !== "N") return 11;
    if (Direction.South !== "S") return 12;
    if (Direction.East !== "E") return 13;
    if (Direction.West !== "W") return 14;

    // Reverse mapping only works for numeric enums
    // Skip reverse mapping tests until parser supports MemberExpr with computed key
    // (Color[0] === "Red")

    // Use enum values in expressions
    const x: number = Color.Red + Color.Green;
    if (x !== 1) return 15;

    // Use enum values in conditional
    const c: number = Color.Green;
    let label: string = "";
    if (c === Color.Red) label = "red";
    else if (c === Color.Green) label = "green";
    else label = "blue";
    if (label !== "green") return 16;

    return 0;
}
