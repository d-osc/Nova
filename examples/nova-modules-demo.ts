// Nova Built-in Modules Combined Demo
// Demonstrates using nova:fs, nova:path, and nova:os together

import { writeFileSync, readFileSync, existsSync } from "nova:fs";
import { dirname, basename } from "nova:path";
import { platform, homedir, tmpdir } from "nova:os";

function main(): number {
    console.log("=== Nova Built-in Modules Demo ===");
    console.log("");

    // Get system information using nova:os
    let plat = platform();
    let home = homedir();
    let tmp = tmpdir();

    console.log("System Information (nova:os):");
    console.log("  Platform: " + plat);
    console.log("  Home: " + home);
    console.log("  Temp: " + tmp);

    // Write and read a file using nova:fs
    console.log("");
    console.log("File Operations (nova:fs):");
    let testFile = "nova_demo_test.txt";

    console.log("  Writing to: " + testFile);
    writeFileSync(testFile, "Hello from Nova!");

    if (existsSync(testFile)) {
        console.log("  File created successfully!");

        let content = readFileSync(testFile);
        console.log("  Content: " + content);
    }

    // Path operations using nova:path
    console.log("");
    console.log("Path Operations (nova:path):");
    let testPath = "/home/user/projects/app.ts";

    let dir = dirname(testPath);
    console.log("  dirname('" + testPath + "')");
    console.log("    -> " + dir);

    let base = basename(testPath);
    console.log("  basename('" + testPath + "')");
    console.log("    -> " + base);

    console.log("");
    console.log("=== Demo Complete ===");
    return 0;
}
