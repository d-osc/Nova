import { randomUUID } from "crypto";

console.log("Calling randomUUID()...");
const uuid = randomUUID();
console.log("UUID type:", typeof uuid);
console.log("UUID value:", uuid);
console.log("UUID length:", uuid ? uuid.length : 0);

if (uuid) {
    console.log("SUCCESS! Got UUID:", uuid);
} else {
    console.log("FAIL: UUID is null or empty");
}
