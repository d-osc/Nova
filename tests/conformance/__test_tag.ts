// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function tag(strings: string[], ...values: number[]): string {
    console.log("inside tag");
    console.log("strings.length=" + strings.length);
    console.log("values.length=" + values.length);
    return "ok";
}

function main(): number {
    console.log("calling tag");
    const r = tag`a=${10}b=${20}c`;
    console.log("r=[" + r + "]");
    return 0;
}
