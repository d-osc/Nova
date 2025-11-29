// Test static properties
class Config {
    static version: number = 100;
    static name: number = 42;

    static getVersion(): number {
        return Config.version;
    }
}

function main(): number {
    let v = Config.version;  // Should be 100

    if (v != 100) {
        return 1;
    }

    let v2 = Config.getVersion();
    if (v2 != 100) {
        return 2;
    }

    return 0;
}
