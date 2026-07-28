// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class NotFoundError extends Error {
    constructor(resource: string) {
        super(`${resource} not found`);
    }
}

function main(): number {
    const error = new NotFoundError("record");
    return error.message === "record not found" ? 0 : 1;
}
