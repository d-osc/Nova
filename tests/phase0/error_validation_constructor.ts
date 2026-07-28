// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class ValidationError extends Error {
    constructor(message: string) {
        super(message);
    }
}

function main(): number {
    try {
        throw new ValidationError("invalid");
    } catch (error) {
        return error instanceof ValidationError ? 0 : 1;
    }
}
