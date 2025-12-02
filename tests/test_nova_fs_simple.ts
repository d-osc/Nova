// Simple test of nova:fs module
import { existsSync } from "nova:fs";

function main(): number {
    // Test existsSync - current directory should exist
    let exists = existsSync(".");
    if (exists) {
        return 0;  // Success
    }
    return 1;  // Failure
}
