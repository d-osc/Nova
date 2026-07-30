// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "Type checking completed successfully"

declare module "@nova/config" {
    export interface PackageConfig {
        name: string;
    }

    export function loadConfig(): PackageConfig;
}

declare global {
    interface NovaWindow {
        title: string;
    }
}

interface NovaWindow {
    build: number;
}

const windowShape: NovaWindow = {
    title: "Nova",
    build: 6
};
