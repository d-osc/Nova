// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "object:fallback/2/true"

function main(): number {
    let source = { name: "nova", count: 2, enabled: true };
    let { name: objectName, missing: objectMissing = "fallback", ...remaining } = source;
    console.log("object:" + objectMissing + "/" + remaining.count + "/" + remaining.enabled);
    return 0;
}
