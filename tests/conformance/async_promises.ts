// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "sync\nthen:42\nawait:43"
// NOVA_EXPECT_STDOUT_CONTAINS: "text:nova!"
// NOVA_EXPECT_STDOUT_CONTAINS: "float:3.5"
// NOVA_EXPECT_STDOUT_CONTAINS: "boolean:true"
// NOVA_EXPECT_STDOUT_CONTAINS: "undefined:true"
// NOVA_EXPECT_STDOUT_CONTAINS: "handled:bad\ncaught:oops\nfinally\nconstructed:made\nconstructed-caught:constructed-bad\nresolved-adopted:inner\nresolver-alias:alias-value\nresolver-escaped:escaped-value\nthen-recovered:ok\ncatch-recovered:fixed\nfinally-value:kept\nthen-adopted:start-next\nrejection-adopted:nested-bad"
// NOVA_EXPECT_STDOUT_CONTAINS: "same:true"
// NOVA_EXPECT_STDOUT_CONTAINS: "executor\nafter-executor\nsame:true\nsync"
// NOVA_EXPECT_STDOUT_CONTAINS: "constructed:made"
// NOVA_EXPECT_STDOUT_CONTAINS: "constructed-caught:constructed-bad"
// NOVA_EXPECT_STDOUT_CONTAINS: "resolved-adopted:inner"
// NOVA_EXPECT_STDOUT_CONTAINS: "then-adopted:start-next"
// NOVA_EXPECT_STDOUT_CONTAINS: "rejection-adopted:nested-bad"
// NOVA_EXPECT_STDOUT_CONTAINS: "all:ab"
// NOVA_EXPECT_STDOUT_CONTAINS: "all-rejected:all-bad"
// NOVA_EXPECT_STDOUT_CONTAINS: "race:winner"
// NOVA_EXPECT_STDOUT_CONTAINS: "any:any-winner"
// NOVA_EXPECT_STDOUT_CONTAINS: "resolver-alias:alias-value"
// NOVA_EXPECT_STDOUT_CONTAINS: "resolver-escaped:escaped-value"

async function answer(): number {
    return 42;
}

async function answerPlusOne(): number {
    let value: number = await answer();
    return value + 1;
}

async function text(): string {
    return "nova";
}

async function decoratedText(): string {
    let value: string = await text();
    return value + "!";
}

async function fraction(): number {
    return 3.5;
}

async function enabled(): boolean {
    return true;
}

function main(): number {
    let first = answer();
    let second = answerPlusOne();
    let third = decoratedText();
    let fourth = fraction();
    let fifth = enabled();
    let sixth = Promise.resolve();

    first.then((value: number): number => {
        console.log("then:" + value);
        return value;
    });
    second.then((value: number): number => {
        console.log("await:" + value);
        return value;
    });
    third.then((value: string): string => {
        console.log("text:" + value);
        return value;
    });
    fourth.then((value: number): number => {
        console.log("float:" + value);
        return value;
    });
    fifth.then((value: boolean): boolean => {
        console.log("boolean:" + value);
        return value;
    });
    sixth.then((value: undefined): undefined => {
        console.log("undefined:" + (value === undefined));
        return value;
    });

    let rejected = Promise.reject("bad");
    let recoveredByThen = rejected.then(undefined, (reason: string): string => {
        console.log("handled:" + reason);
        return "ok";
    });
    recoveredByThen.then((value: string): string => {
        console.log("then-recovered:" + value);
        return value;
    });

    let rejectedAgain = Promise.reject("oops");
    let recoveredByCatch = rejectedAgain.catch((reason: string): string => {
        console.log("caught:" + reason);
        return "fixed";
    });
    recoveredByCatch.then((value: string): string => {
        console.log("catch-recovered:" + value);
        return value;
    });

    let kept = Promise.resolve("kept");
    let finalized = kept.finally((): void => {
        console.log("finally");
    });
    finalized.then((value: string): string => {
        console.log("finally-value:" + value);
        return value;
    });

    let constructed = new Promise((resolve, reject): void => {
        console.log("executor");
        resolve("made");
        reject("ignored");
    });
    constructed.then((value: string): string => {
        console.log("constructed:" + value);
        return value;
    });

    let constructedRejected = new Promise((resolve, reject): void => {
        reject("constructed-bad");
    });
    constructedRejected.catch((reason: string): string => {
        console.log("constructed-caught:" + reason);
        return "recovered";
    });
    console.log("after-executor");

    console.log("same:" + (Promise.resolve(first) === first));

    let resolvedAdopted = new Promise((resolve, reject): void => {
        resolve(Promise.resolve("inner"));
    });
    resolvedAdopted.then((value: string): string => {
        console.log("resolved-adopted:" + value);
        return value;
    });

    let thenAdopted = Promise.resolve("start").then((value: string) => {
        return Promise.resolve(value + "-next");
    });
    thenAdopted.then((value: string): string => {
        console.log("then-adopted:" + value);
        return value;
    });

    let rejectionAdopted = Promise.resolve("go").then((value: string) => {
        return Promise.reject("nested-bad");
    });
    rejectionAdopted.catch((reason: string): string => {
        console.log("rejection-adopted:" + reason);
        return "handled";
    });

    let allValues = Promise.all([
        Promise.resolve("a"), Promise.resolve("b")
    ]);
    allValues.then((values: any[]): any[] => {
        console.log("all:" + values[0] + values[1]);
        return values;
    });

    let allRejected = Promise.all([
        Promise.resolve("ok"), Promise.reject("all-bad")
    ]);
    allRejected.catch((reason: string): string => {
        console.log("all-rejected:" + reason);
        return "handled";
    });

    Promise.race([
        Promise.resolve("winner"), Promise.reject("loser")
    ]).then((value: string): string => {
        console.log("race:" + value);
        return value;
    });

    Promise.any([
        Promise.reject("first-bad"), Promise.resolve("any-winner")
    ]).then((value: string): string => {
        console.log("any:" + value);
        return value;
    });

    let resolverAliasPromise = new Promise((resolve, reject): void => {
        let resolverAlias = resolve;
        resolverAlias("alias-value");
    });
    resolverAliasPromise.then((value: string): string => {
        console.log("resolver-alias:" + value);
        return value;
    });

    let savedResolve: any;
    let resolverEscapedPromise = new Promise((resolve, reject): void => {
        savedResolve = resolve;
    });
    savedResolve("escaped-value");
    resolverEscapedPromise.then((value: string): string => {
        console.log("resolver-escaped:" + value);
        return value;
    });

    console.log("sync");
    return 0;
}
