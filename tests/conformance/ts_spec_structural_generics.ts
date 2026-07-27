// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

interface Identified {
    readonly id: number;
}

interface Box<T = string> {
    value: T;
}

type Pair<Left, Right> = {
    left: Left;
    right: Right;
};

function identity<T>(value: T): T {
    return value;
}

function property<T, Key extends keyof T>(value: T, key: Key): T[Key] {
    return value[key];
}

function withId<T extends Identified>(value: T): T {
    return value;
}

const defaultBox: Box = { value: "nova" };
const numberBox: Box<number> = { value: 42 };
const pair: Pair<string, number> = { left: "answer", right: 42 };
const item = withId({ id: 1, label: "one" });

const inferredNumber: number = identity(42);
const inferredString: string = identity("nova");
const selectedId: number = property(item, "id");
const selectedLabel: string = property(item, "label");
