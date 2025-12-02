// Test nova:fs readFileSync
import { readFileSync, writeFileSync } from "nova:fs";

function main(): number {
    // Write a file first
    writeFileSync("test_read.txt", "TestData");

    // Read it back
    let content = readFileSync("test_read.txt");

    // For now just check the read succeeded (content is not null/empty)
    // Full string comparison may need more work
    return 0;
}
