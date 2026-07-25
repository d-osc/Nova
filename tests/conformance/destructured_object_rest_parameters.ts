// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "parameter-rest:nova/0/true"
// NOVA_EXPECT_STDOUT_CONTAINS: "arrow-rest:two/false"

function readRest({ keep, ...rest }): number {
    console.log("parameter-rest:" + rest.name + "/" + rest.zero + "/" + rest.enabled);
    return keep;
}

function main(): number {
    readRest({ name: "nova", keep: 1, zero: 0, enabled: true });
    let readArrow = ({ first, ...remaining }) => {
        console.log("arrow-rest:" + remaining.second + "/" + remaining.enabled);
    };
    readArrow({ enabled: false, second: "two", first: 1 });
    return 0;
}
