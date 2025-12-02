// Nova OS Module Demo
// Demonstrates nova:os built-in module for operating system utilities

import { platform, arch, homedir, tmpdir, hostname } from "nova:os";

function main(): number {
    console.log("=== Nova OS Module Demo ===");
    console.log("");

    // 1. Platform
    console.log("1. platform()");
    let plat = platform();
    console.log("   Result: " + plat);

    // 2. Architecture
    console.log("");
    console.log("2. arch()");
    let architecture = arch();
    console.log("   Result: " + architecture);

    // 3. Home directory
    console.log("");
    console.log("3. homedir()");
    let home = homedir();
    console.log("   Result: " + home);

    // 4. Temp directory
    console.log("");
    console.log("4. tmpdir()");
    let tmp = tmpdir();
    console.log("   Result: " + tmp);

    // 5. Hostname
    console.log("");
    console.log("5. hostname()");
    let host = hostname();
    console.log("   Result: " + host);

    console.log("");
    console.log("=== System Info Summary ===");
    console.log("Platform:  " + plat);
    console.log("Arch:      " + architecture);
    console.log("Home:      " + home);
    console.log("Temp:      " + tmp);
    console.log("Hostname:  " + host);

    console.log("");
    console.log("=== Demo Complete ===");
    return 0;
}
