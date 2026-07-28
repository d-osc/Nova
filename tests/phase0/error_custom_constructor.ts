// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class CustomError extends Error {
    constructor(message: string) {
        super(message);
    }
}

function main(): number {
    const error = new CustomError("custom");
    return error.message === "custom" ? 0 : 1;
}
