// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 1
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2322: Type 'string' is not assignable to type 'number'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2345: Argument of type 'string' is not assignable to parameter of type 'number'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2322: Type 'bigint' is not assignable to type 'number'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2322: Type 'symbol' is not assignable to type 'string'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2365: Operator cannot be applied to types 'bigint' and 'number'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2469: The operator cannot be applied to type 'symbol'."
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2339: Property 'missing' does not exist on type 'RequiredShape'."
// NOVA_EXPECT_STDERR_CONTAINS: "does not satisfy the constraint 'RequiredShape'."

function identity(value: number): number {
    return value;
}

let count: number = "wrong";
identity("wrong");
let invalidUnion: number | string = false;
let invalidBigInt: number = 1n;
let invalidSymbol: string = Symbol("typed");
let mixedBigInt = 1n + 1;
let mathSymbol: symbol = Symbol("math");
let invalidSymbolMath = +mathSymbol;

interface RequiredShape {
    value: number;
    label?: string;
}
let missingRequired: RequiredShape = { label: "missing value" };
let wrongRequired: RequiredShape = { value: "wrong" };
let validRequired: RequiredShape = { value: 1 };
validRequired.missing;

function genericIdentity<T>(value: T): T {
    return value;
}
let wrongGeneric: string = genericIdentity(42);

function requiredValue<T extends RequiredShape>(value: T): number {
    return value.value;
}
requiredValue({ label: "still missing" });
