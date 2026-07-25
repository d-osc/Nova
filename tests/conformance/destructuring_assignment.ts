// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "array-assignment:1/two/true"
// NOVA_EXPECT_STDOUT_CONTAINS: "object-assignment:nova/2"
// NOVA_EXPECT_STDOUT_CONTAINS: "nested-assignment:3/4/ok"
// NOVA_EXPECT_STDOUT_CONTAINS: "default-assignment:fallback/null"
// NOVA_EXPECT_STDOUT_CONTAINS: "object-rest-assignment:value/true"

function main(): number {
    let first: any = 0;
    let second: any = "";
    let tail: any[] = [];
    [first, second, ...tail] = [1, "two", true];
    console.log("array-assignment:" + first + "/" + second + "/" + tail[0]);

    let label: any = "";
    let count: any = 0;
    ({ name: label, count } = { count: 2, name: "nova" });
    console.log("object-assignment:" + label + "/" + count);

    let x: any = 0;
    let y: any = 0;
    let nestedLabel: any = "";
    ({ point: [x, y], meta: { label: nestedLabel } } =
        { point: [3, 4], meta: { label: "ok" } });
    console.log("nested-assignment:" + x + "/" + y + "/" + nestedLabel);

    let missing: any = "";
    let preserved: any = "";
    [missing = "fallback", preserved = "fallback"] = [undefined, null];
    console.log("default-assignment:" + missing + "/" + preserved);

    let keep: any = 0;
    let remaining = { extra: "" };
    ({ keep, ...remaining } = { enabled: true, extra: "value", keep: 1 });
    console.log("object-rest-assignment:" + remaining.extra + "/" + remaining.enabled);
    return 0;
}
