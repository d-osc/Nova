// Nova Path Module Demo
// Demonstrates nova:path built-in module for path manipulation

import { dirname, basename, extname, resolve, normalize } from "nova:path";

function main(): number {
    console.log("=== Nova Path Module Demo ===");
    console.log("");

    // 1. Get directory name
    console.log("1. dirname('/home/user/projects/app/index.ts')");
    let dir = dirname("/home/user/projects/app/index.ts");
    console.log("   Result: " + dir);

    // 2. Get base name
    console.log("");
    console.log("2. basename('/home/user/projects/app/index.ts')");
    let base = basename("/home/user/projects/app/index.ts");
    console.log("   Result: " + base);

    // 3. Get extension
    console.log("");
    console.log("3. extname('package.json')");
    let ext = extname("package.json");
    console.log("   Result: " + ext);

    // 4. Get more extensions
    console.log("");
    console.log("4. extname('app.config.ts')");
    let ext2 = extname("app.config.ts");
    console.log("   Result: " + ext2);

    // 5. Resolve to absolute path
    console.log("");
    console.log("5. resolve('.')");
    let abs = resolve(".");
    console.log("   Result: " + abs);

    // 6. Normalize path
    console.log("");
    console.log("6. normalize('src/../lib/./utils')");
    let norm = normalize("src/../lib/./utils");
    console.log("   Result: " + norm);

    // 7. Dirname of different paths
    console.log("");
    console.log("7. dirname('C:/Users/test/file.txt')");
    let dir2 = dirname("C:/Users/test/file.txt");
    console.log("   Result: " + dir2);

    console.log("");
    console.log("=== Demo Complete ===");
    return 0;
}
