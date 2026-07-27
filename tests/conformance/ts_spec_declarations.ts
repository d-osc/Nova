// NOVA_TEST_MODE: compile
// NOVA_EXPECT_EXIT: 0

declare const VERSION: string;
declare function readEnvironment(name: string): string | undefined;

declare namespace Runtime {
    interface Options {
        debug?: boolean;
    }

    function start(options?: Options): void;
}

declare class NativeResource {
    readonly handle: number;
    close(): void;
}

type EnvironmentValue = ReturnType<typeof readEnvironment>;
type RuntimeOptions = Runtime.Options;
