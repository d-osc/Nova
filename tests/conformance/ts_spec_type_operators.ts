// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

type ArrayElement<Type> = Type extends readonly (infer Item)[] ? Item : never;
type AwaitedValue<Type> = Type extends Promise<infer Value> ? Value : Type;

interface Model {
    id: number;
    name: string;
    active?: boolean;
}

type ReadonlyModel = {
    readonly [Key in keyof Model]-?: Model[Key];
};

type Getters<Type> = {
    [Key in keyof Type as `get${Capitalize<string & Key>}`]: () => Type[Key];
};

type Events = "open" | "close";
type EventHandlerName = `on${Capitalize<Events>}`;
type StringKeys = Extract<keyof Model, string>;
type ModelPatch = Partial<Model>;
type RequiredModel = Required<Model>;
type PublicModel = Pick<Model, "id" | "name">;
type ModelWithoutId = Omit<Model, "id">;
type ModelRecord = Record<"primary" | "secondary", Model>;

const numberElement: ArrayElement<number[]> = 1;
const awaitedText: AwaitedValue<Promise<string>> = "ready";
const handlerName: EventHandlerName = "onOpen";
const key: StringKeys = "name";
const patch: ModelPatch = { active: true };
const publicModel: PublicModel = { id: 1, name: "nova" };
const withoutId: ModelWithoutId = { name: "nova" };
const records: ModelRecord = {
    primary: { id: 1, name: "one" },
    secondary: { id: 2, name: "two", active: true }
};
