// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "{\"name\":\"Nova\",\"count\":3,\"active\":true}"

function main(): number {
    let object = { name: "Nova", count: 3, active: true };
    let json = JSON.stringify(object);
    console.log(json);
    if (json != "{\"name\":\"Nova\",\"count\":3,\"active\":true}") return 1;
    return 0;
}
