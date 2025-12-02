// Test nova:fs writeFileSync and existsSync
import { writeFileSync, existsSync } from "nova:fs";

function main(): number {
    // Write a file
    writeFileSync("test_nova_write.txt", "Hello");

    // Check if file exists after writing
    if (existsSync("test_nova_write.txt")) {
        return 0;  // Success
    }
    return 1;  // Failure
}
