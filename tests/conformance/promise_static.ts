// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "all:6"
// NOVA_EXPECT_STDOUT_CONTAINS: "race:fast"
// NOVA_EXPECT_STDOUT_CONTAINS: "allSettled:3"
// NOVA_EXPECT_STDOUT_CONTAINS: "any:1"
// NOVA_EXPECT_STDOUT_CONTAINS: "resolve-same:true"

// Test Promise static methods: all, race, allSettled, any, withResolvers, resolve, reject

function main(): number {
    // Promise.resolve / Promise.reject
    const resolved = Promise.resolve(42);
    resolved.then((v: number) => {
        if (v === 42) console.log("resolve:42");
    });

    // Promise.all — happy path
    Promise.all([Promise.resolve(1), Promise.resolve(2), Promise.resolve(3)])
        .then((vals: number[]) => {
            const sum = vals[0] + vals[1] + vals[2];
            console.log("all:" + sum);
        });

    // Promise.all — rejects on any rejection
    Promise.all([Promise.resolve("a"), Promise.reject("b")])
        .catch((err: string) => {
            console.log("all-reject:" + err);
        });

    // Promise.race — first to settle wins
    Promise.race([
        Promise.resolve("fast"),
        Promise.reject("slow-error")
    ]).then((v: string) => {
        console.log("race:" + v);
    });

    // Promise.allSettled — never rejects, returns status objects
    Promise.allSettled([
        Promise.resolve(1),
        Promise.reject("bad"),
        Promise.resolve(3)
    ]).then((results: any[]) => {
        let fulfilled = 0;
        let sum = 0;
        for (const r of results) {
            if (r.status === "fulfilled") {
                fulfilled++;
                sum += r.value;
            }
        }
        // allSettled returns one result per input, including rejections.
        // Verify both the aggregate length and the fulfilled subset.
        if (fulfilled === 2) {
            console.log("allSettled:" + results.length);
        }
    });

    // Promise.any — first fulfilled wins, throws AggregateError on all-reject
    Promise.any([
        Promise.reject("e1"),
        Promise.resolve(1),
        Promise.resolve(2)
    ]).then((v: number) => {
        console.log("any:" + v);
    });

    // Promise.all with non-promise values (auto-wrapped)
    Promise.all([10, 20, 30]).then((vals: number[]) => {
        console.log("all-mix:" + (vals[0] + vals[1] + vals[2]));
    });

    // Promise.resolve identity — returns same promise if already a Promise
    const existing = Promise.resolve("x");
    const same = Promise.resolve(existing);
    if (existing === same) console.log("resolve-same:true");

    // withResolvers
    const { promise, resolve, reject } = Promise.withResolvers();
    resolve("with-resolvers-ok");
    promise.then((v: string) => {
        console.log("wr:" + v);
    });

    // Empty Promise.all resolves to empty array
    Promise.all([]).then((vals: any[]) => {
        console.log("all-empty:" + vals.length);
    });

    // Empty Promise.race never resolves (we won't test that to avoid hang)
    // Instead, single-element race
    Promise.race([Promise.resolve("solo")]).then((v: string) => {
        console.log("race-solo:" + v);
    });

    return 0;
}
