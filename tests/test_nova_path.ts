// Test nova:path module
import { join, dirname, basename, extname } from "nova:path";

function main(): number {
    // Test join
    let joined = join("dir", "file.txt");

    // Test dirname
    let dir = dirname("/path/to/file.txt");

    // Test basename
    let base = basename("/path/to/file.txt");

    // Test extname
    let ext = extname("file.txt");

    // All functions should execute without crashing
    return 0;
}
