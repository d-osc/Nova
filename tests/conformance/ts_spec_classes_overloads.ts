// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

interface Serializable {
    serialize(): string;
}

abstract class Entity implements Serializable {
    static count: number = 0;
    readonly id: number;
    protected label: string;
    private secret: string;

    constructor(id: number, label: string) {
        this.id = id;
        this.label = label;
        this.secret = "hidden";
        Entity.count++;
    }

    abstract serialize(): string;

    protected displayLabel(): string {
        return this.label;
    }
}

class User extends Entity {
    override serialize(): string {
        return this.displayLabel() + ":" + this.id;
    }
}

function combine(left: string, right: string): string;
function combine(left: number, right: number): number;
function combine(left: string | number, right: string | number): string | number {
    if (typeof left === "string" && typeof right === "string") {
        return left + right;
    }
    return (left as number) + (right as number);
}

const user: Serializable = new User(1, "nova");
const text: string = combine("no", "va");
const total: number = combine(20, 22);
