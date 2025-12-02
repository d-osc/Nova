// Nova File System Module Demo
// Demonstrates nova:fs built-in module for file operations

import { readFileSync, writeFileSync, existsSync, mkdirSync, readdirSync, unlinkSync } from "nova:fs";

function main(): number {
    console.log("=== Nova File System Demo ===\n");

    // 1. Create a directory
    console.log("1. Creating directory 'demo_output'...");
    mkdirSync("demo_output");

    // 2. Write a file
    console.log("2. Writing file 'demo_output/hello.txt'...");
    writeFileSync("demo_output/hello.txt", "Hello from Nova!");

    // 3. Check if file exists
    console.log("3. Checking if file exists...");
    if (existsSync("demo_output/hello.txt")) {
        console.log("   File exists!");
    } else {
        console.log("   File not found!");
        return 1;
    }

    // 4. Read file content
    console.log("4. Reading file content...");
    let content = readFileSync("demo_output/hello.txt");
    console.log("   Content: " + content);

    // 5. Write another file
    console.log("5. Writing 'demo_output/data.json'...");
    writeFileSync("demo_output/data.json", '{"name": "Nova", "version": "1.0"}');

    // 6. List directory contents
    console.log("6. Listing directory contents...");
    let files = readdirSync("demo_output");
    console.log("   Files in demo_output/");

    // 7. Clean up - delete files
    console.log("7. Cleaning up...");
    unlinkSync("demo_output/hello.txt");
    unlinkSync("demo_output/data.json");

    console.log("\n=== Demo Complete ===");
    return 0;
}
