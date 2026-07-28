// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class ApplicationError extends Error {
    constructor(message, code) {
        super(message);
        this.code = code;
    }
}

function main() {
    const error = new ApplicationError("failed", 500);
    return error instanceof ApplicationError &&
        error instanceof Error &&
        error.code === 500 ? 0 : 1;
}
