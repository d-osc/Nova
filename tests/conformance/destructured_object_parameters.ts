// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "object-param:nova/2/fallback"

function readObject({ name, nested: { count }, missing = "fallback" }): number {
    console.log("object-param:" + name + "/" + count + "/" + missing);
    return 0;
}

function main(): number {
    readObject({ nested: { count: 2 }, name: "nova" });
    return 0;
}
