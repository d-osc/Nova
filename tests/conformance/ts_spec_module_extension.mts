// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

export type Identifier = number;

export function identity<T>(value: T): T {
    return value;
}

function main(): number {
    const value: Identifier = identity(42);
    return value === 42 ? 0 : 1;
}
