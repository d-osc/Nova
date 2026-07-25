// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "arrow-object:nova/2/fallback"
// NOVA_EXPECT_STDOUT_CONTAINS: "arrow-array:1/two"

function main(): number {
    let readObject = ({ name, nested: { count }, missing = "fallback" }) => {
        console.log("arrow-object:" + name + "/" + count + "/" + missing);
    };
    let readArray = ([first, second]) => {
        console.log("arrow-array:" + first + "/" + second);
    };
    readObject({ nested: { count: 2 }, name: "nova" });
    readArray([1, "two"]);
    return 0;
}
