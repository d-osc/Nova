// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

function add(left: number, right: number): number {
    return left + right;
}

let total: number = add(2, 3);
let label: string = "total";
let flexible: number | string = 1;
flexible = "now a string";
let values: number[] = [1, 2, 3];
let large: bigint = 12345678901234567890n;
let tokenSymbol: symbol = Symbol("typed");

function addLarge(left: bigint, right: bigint): bigint {
    return left + right;
}
let negativeLarge: bigint = -large;
let incrementedLarge: bigint = addLarge(large, 1n);

interface Point {
    x: number;
    label?: string;
    distance(other: Point): number;
}

let point: Point = {
    x: 1,
    distance(other: Point): number { return other.x; }
};
let pointX: number = point.x;
let pointLabel: string | undefined = point.label;

type ServerConfig = { port: number; secure?: boolean };
let server: ServerConfig = { port: 8080 };
let checkedServer = { port: 443, secure: true } satisfies ServerConfig;

interface NamedPoint extends Point {
    name: string;
}
let namedPoint: NamedPoint = {
    x: 2,
    name: "origin",
    distance(other: Point): number { return other.x; }
};

function genericIdentity<T>(value: T): T {
    return value;
}
let genericNumber: number = genericIdentity(42);
let genericString: string = genericIdentity("nova");

function pointXOf<T extends Point>(value: T): number {
    return value.x;
}
let constrainedGeneric: number = pointXOf(namedPoint);
