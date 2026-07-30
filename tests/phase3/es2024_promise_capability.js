// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const capability = Promise.withResolvers();
    if (!(capability.promise instanceof Promise)) return 1;
    if (typeof capability.resolve !== "function") return 2;
    if (typeof capability.reject !== "function") return 3;
    capability.resolve("ok");
    return 0;
}
