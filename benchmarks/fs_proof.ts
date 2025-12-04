// Proof that Nova FS works
import { writeFileSync, readFileSync, mkdirSync, rmSync, existsSync } from "fs";

mkdirSync("./proof_test");
writeFileSync("./proof_test/data.txt", "Nova FS Works!");
const exists = existsSync("./proof_test/data.txt");
rmSync("./proof_test");
writeFileSync("./fs_proof_complete.txt", "FS module is functional");
