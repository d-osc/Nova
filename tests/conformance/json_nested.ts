// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "{\"title\":\"A\\\"B\\\\C\\n\",\"flags\":[true,false],\"tags\":[\"one\",\"two\"],\"child\":{\"count\":2,\"ok\":true}}"

function main(): number {
    let value = {
        title: "A\"B\\C\n",
        flags: [true, false],
        tags: ["one", "two"],
        child: { count: 2, ok: true }
    };
    let json = JSON.stringify(value);
    console.log(json);
    if (json != "{\"title\":\"A\\\"B\\\\C\\n\",\"flags\":[true,false],\"tags\":[\"one\",\"two\"],\"child\":{\"count\":2,\"ok\":true}}") return 1;
    return 0;
}
