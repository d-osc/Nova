// Test nova:os module
import { platform, arch, homedir, tmpdir } from "nova:os";

function main(): number {
    // Test platform
    let p = platform();

    // Test arch
    let a = arch();

    // Test homedir
    let home = homedir();

    // Test tmpdir
    let tmp = tmpdir();

    // All functions should execute without crashing
    return 0;
}
