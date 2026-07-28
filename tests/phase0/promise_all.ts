// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "all:6"

function main(): number {
    Promise.resolve(42).then((value: number) => {
        if (value === 42) console.log("resolve:42");
    });
    Promise.all([Promise.resolve(1), Promise.resolve(2), Promise.resolve(3)])
        .then((values: number[]) =>
            console.log("all:" + (values[0] + values[1] + values[2])));
    Promise.all([Promise.resolve("a"), Promise.reject("b")])
        .catch((error: string) => console.log("reject:" + error));
    Promise.race([Promise.resolve("fast"), Promise.reject("slow")])
        .then((value: string) => console.log("race:" + value));
    Promise.allSettled([
        Promise.resolve(1), Promise.reject("bad"), Promise.resolve(3)
    ]).then((results: any[]) => {
        let fulfilled = 0;
        for (const result of results) {
            if (result.status === "fulfilled") fulfilled++;
        }
        console.log("settled:" + fulfilled);
    });
    return 0;
}
