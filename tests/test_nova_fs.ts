// Test nova:fs module
import { readFileSync, writeFileSync, existsSync, mkdirSync } from "nova:fs";

function main(): number {
    let errors = 0;

    // Test writeFileSync and readFileSync
    let testContent = "Hello from Nova!";
    writeFileSync("test_output.txt", testContent);

    if (!existsSync("test_output.txt")) {
        errors = errors + 1;
    }

    let readContent = readFileSync("test_output.txt");
    if (readContent !== testContent) {
        errors = errors + 1;
    }

    // Clean up
    // unlinkSync("test_output.txt");

    return errors;
}
