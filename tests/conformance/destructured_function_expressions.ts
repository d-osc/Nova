// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "function-expr:nova/2/fallback"
// NOVA_EXPECT_STDOUT_CONTAINS: "function-array:1/two"

function main(): number {
    let readObject = function({ name, nested: { count }, missing = "fallback" }): number {
        console.log("function-expr:" + name + "/" + count + "/" + missing);
        return 0;
    };
    let readArray = function([first, second]): number {
        console.log("function-array:" + first + "/" + second);
        return 0;
    };
    readObject({ nested: { count: 2 }, name: "nova" });
    readArray([1, "two"]);
    return 0;
}
