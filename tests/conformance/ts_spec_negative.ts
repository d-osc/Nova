// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 1
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2322"
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2540"
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2344"
// NOVA_EXPECT_STDERR_CONTAINS: "error TS2769"

interface Configuration {
    readonly port: number;
    mode: "development" | "production";
}

const configuration: Configuration = {
    port: "8080",
    mode: "development"
};

configuration.port = 9000;

type StringOnly<T extends string> = T;
type InvalidConstraint = StringOnly<number>;

function convert(value: string): string;
function convert(value: number): number;
function convert(value: string | number): string | number {
    return value;
}

convert(true);
