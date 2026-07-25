// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "async-arrow:nova/2/fallback"

function main(): number {
    let readObject = async ({ name, nested: { count }, missing = "fallback" }) => {
        console.log("async-arrow:" + name + "/" + count + "/" + missing);
        return count;
    };
    readObject({ nested: { count: 2 }, name: "nova" });
    return 0;
}
