// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

type Result =
    | { kind: "ok"; value: number }
    | { kind: "error"; message: string };

function render(result: Result): string {
    if (result.kind === "ok") {
        return result.value.toFixed(2);
    }
    return result.message.toUpperCase();
}

function measureLength(value: string | string[] | null): number {
    if (value === null) return 0;
    if (typeof value === "string") return value.length;
    return value.length;
}

interface Fish { swim(): void; }
interface Bird { fly(): void; }

function move(animal: Fish | Bird): void {
    if ("swim" in animal) animal.swim();
    else animal.fly();
}

function isString(value: unknown): value is string {
    return typeof value === "string";
}

function assertNever(value: never): never {
    throw new Error("Unexpected value: " + value);
}

function exhaustive(result: Result): number {
    switch (result.kind) {
        case "ok": return result.value;
        case "error": return result.message.length;
        default: return assertNever(result);
    }
}

const text: string = render({ kind: "ok", value: 1 });
const size: number = measureLength(["a", "b"]);
const narrowed: string = isString("nova") ? "nova" : "";
