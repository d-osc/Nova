// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test optional chaining (?.) - null/undefined short-circuit

function main(): number {
    // Setup an object with nested nulls
    const obj = {
        a: { b: { c: 42 } },
        nothing: null as any
    };

    // Basic optional member access - returns value when present
    if (obj.a?.b?.c !== 42) return 1;

    // Short-circuit on null/undefined - whole chain becomes undefined
    const val = obj.nothing?.x;
    if (val !== undefined && val !== null) return 2;

    // Optional chaining on undefined intermediate
    const obj2: any = { x: undefined };
    const r = obj2.x?.y?.z;
    if (r !== undefined && r !== null) return 3;

    // Optional call on a function value
    const fn = (n: number) => n * 2;
    if (fn?.(5) !== 10) return 4;

    // Optional call on null/undefined - returns undefined
    const nullFn: any = null;
    const callResult = nullFn?.();
    if (callResult !== undefined && callResult !== null) return 5;

    // Optional method call on object that has the method
    const arr = [1, 2, 3];
    const pushed = arr.push?.(4);
    if (arr.length !== 4) return 6;

    // Optional access on null literal directly
    const direct = (null as any)?.foo;
    if (direct !== undefined && direct !== null) return 7;

    // Chained optional access with mix of . and ?.
    const data: any = { users: [{ name: "alice" }] };
    const name = data?.users?.[0]?.name;
    if (name !== "alice") return 8;

    // Optional on missing property of present object -> should be undefined
    const missing = obj.a?.nonexistent?.deeper;
    if (missing !== undefined && missing !== null) return 9;

    return 0;
}
