// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "dynamic-this-call:7"
// NOVA_EXPECT_STDOUT_CONTAINS: "dynamic-this-apply:8"
// NOVA_EXPECT_STDOUT_CONTAINS: "dynamic-this-bind:9"

function receiverValue(): any {
    return this;
}

function main(): number {
    let called: any = receiverValue.call(7);
    let applied: any = receiverValue.apply(8, []);
    let receiver: number = 9;
    let bound = receiverValue.bind(receiver);
    receiver = 10;

    console.log("dynamic-this-call:" + called);
    console.log("dynamic-this-apply:" + applied);
    console.log("dynamic-this-bind:" + bound());
    return 0;
}
