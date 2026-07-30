function main() {
  const zod = require("zod");
  const value = zod.z.string().parse("nova");
  console.log("zod-ok", value);
  return value === "nova" ? 0 : 1;
}
