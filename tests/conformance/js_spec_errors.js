// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const cause = new Error("root");
    const error = new TypeError("outer", { cause });
    if (error.name !== "TypeError" || error.message !== "outer") return 1;
    if (error.cause !== cause) return 2;
    if (!(error instanceof Error) || !(error instanceof TypeError)) return 3;

    const aggregate = new AggregateError(
        [new RangeError("range"), new SyntaxError("syntax")],
        "many",
        { cause }
    );
    if (aggregate.name !== "AggregateError") return 4;
    if (aggregate.message !== "many" || aggregate.cause !== cause) return 5;
    if (aggregate.errors.length !== 2) return 6;
    if (!(aggregate.errors[0] instanceof RangeError)) return 7;

    class ApplicationError extends Error {
        constructor(message, code) {
            super(message);
            this.name = "ApplicationError";
            this.code = code;
        }
    }
    const custom = new ApplicationError("failed", 500);
    if (!(custom instanceof ApplicationError) || !(custom instanceof Error)) return 8;
    if (custom.name !== "ApplicationError" || custom.code !== 500) return 9;
    if (typeof custom.stack !== "string" || custom.stack.length === 0) return 10;

    return 0;
}
