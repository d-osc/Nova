// Nova FS Basic Test
// Tests core nova:fs functions

import {
    readFileSync,
    writeFileSync,
    existsSync,
    mkdirSync,
    readdirSync,
    unlinkSync,
    appendFileSync,
    copyFileSync,
    renameSync,
    rmdirSync
} from "nova:fs";

function main(): number {
    console.log("=== Nova FS Basic Test ===");
    console.log("");

    // 1. Create directory
    console.log("1. mkdirSync('fs_test')");
    mkdirSync("fs_test");
    console.log("   Done");

    // 2. Check exists
    console.log("");
    console.log("2. existsSync('fs_test')");
    if (existsSync("fs_test")) {
        console.log("   Result: true");
    } else {
        console.log("   Result: false");
    }

    // 3. Write file
    console.log("");
    console.log("3. writeFileSync('fs_test/hello.txt', 'Hello World')");
    writeFileSync("fs_test/hello.txt", "Hello World");
    console.log("   Done");

    // 4. Read file
    console.log("");
    console.log("4. readFileSync('fs_test/hello.txt')");
    let content = readFileSync("fs_test/hello.txt");
    console.log("   Content: " + content);

    // 5. Append to file
    console.log("");
    console.log("5. appendFileSync('fs_test/hello.txt', ' - Nova')");
    appendFileSync("fs_test/hello.txt", " - Nova");
    let content2 = readFileSync("fs_test/hello.txt");
    console.log("   Content: " + content2);

    // 6. Copy file
    console.log("");
    console.log("6. copyFileSync('fs_test/hello.txt', 'fs_test/copy.txt')");
    copyFileSync("fs_test/hello.txt", "fs_test/copy.txt");
    if (existsSync("fs_test/copy.txt")) {
        console.log("   Copy exists: true");
    } else {
        console.log("   Copy exists: false");
    }

    // 7. Rename file
    console.log("");
    console.log("7. renameSync('fs_test/copy.txt', 'fs_test/renamed.txt')");
    renameSync("fs_test/copy.txt", "fs_test/renamed.txt");
    if (existsSync("fs_test/renamed.txt")) {
        console.log("   Renamed exists: true");
    } else {
        console.log("   Renamed exists: false");
    }

    // 8. List directory
    console.log("");
    console.log("8. readdirSync('fs_test')");
    let files = readdirSync("fs_test");
    console.log("   Files: " + files);

    // 9. Delete files
    console.log("");
    console.log("9. unlinkSync (delete files)");
    unlinkSync("fs_test/hello.txt");
    unlinkSync("fs_test/renamed.txt");
    console.log("   Deleted");

    // 10. Remove directory
    console.log("");
    console.log("10. rmdirSync('fs_test')");
    rmdirSync("fs_test");
    if (!existsSync("fs_test")) {
        console.log("    Directory removed");
    }

    console.log("");
    console.log("=== All Tests Complete ===");
    return 0;
}
