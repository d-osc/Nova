// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "defaults:fallback/null/fallback"

function main(): number {
    function defaultValue(): string {
        return "fallback";
    }
    let values: any[] = [undefined, null];
    let [missing = defaultValue(), preserved = defaultValue(), absent = defaultValue()] = values;
    console.log("defaults:" + missing + "/" + preserved + "/" + absent);
    return 0;
}
