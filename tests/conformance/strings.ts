// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let value = "Nova Compiler";
    if (value.length != 13) return 1;
    if (value.charCodeAt(0) != 78) return 2;
    if (!value.includes("Compiler")) return 3;
    return 0;
}
