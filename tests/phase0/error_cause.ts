// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    try {
        try {
            throw new Error("root cause");
        } catch (root) {
            throw new Error("wrapped", { cause: root });
        }
    } catch (error) {
        const wrapped = error as any;
        if (wrapped.message !== "wrapped") return 1;
        if (!wrapped.cause) return 2;
        if (wrapped.cause.message !== "root cause") return 3;
    }
    return 0;
}
