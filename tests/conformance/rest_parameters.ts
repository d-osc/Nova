// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "rest-count:3\n10\nx\ntrue\nresult:h3"
// NOVA_EXPECT_STDOUT_CONTAINS: "rest-count:0\nresult:e0"
// NOVA_EXPECT_STDOUT_CONTAINS: "forward-count:2\nrest-count:2\na\nb\nresult:f2"
// NOVA_EXPECT_STDOUT_CONTAINS: "arrow-count:2\nq\narrow:q2"
// NOVA_EXPECT_STDOUT_CONTAINS: "expr-count:2\n1\nexpr:p2"
// NOVA_EXPECT_STDOUT_CONTAINS: "async-count:2\nz\nasync-result:done"

function collect(prefix: string, ...items: any[]): string {
    console.log("rest-count:" + items.length);
    if (items.length > 0) console.log(items[0]);
    if (items.length > 1) console.log(items[1]);
    if (items.length > 2) console.log(items[2]);
    return prefix + items.length;
}

function forward(...values: any[]): string {
    console.log("forward-count:" + values.length);
    return collect("f", values[0], values[1]);
}

function main(): number {
    console.log("result:" + collect("h", 10, "x", true));
    console.log("result:" + collect("e"));
    console.log("result:" + forward("a", "b"));

    let arrow = (...values: any[]): string => {
        console.log("arrow-count:" + values.length);
        console.log(values[0]);
        return "q2";
    };
    console.log("arrow:" + arrow("q", 7));

    let expression = function(prefix: string, ...values: any[]): string {
        console.log("expr-count:" + values.length);
        console.log(values[0]);
        return "p2";
    };
    console.log("expr:" + expression("p", 1, 2));

    let asyncArrow = async (...values: any[]): string => {
        console.log("async-count:" + values.length);
        console.log(values[0]);
        return "done";
    };
    let asyncPromise = asyncArrow("z", 9);
    asyncPromise.then((value: string): string => {
        console.log("async-result:" + value);
        return value;
    });
    return 0;
}
