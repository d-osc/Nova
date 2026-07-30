function main() {
  const lodash = require("lodash");
  const chunks = lodash.chunk([1, 2, 3], 2);
  console.log("lodash-ok", chunks.length);
  return chunks.length === 2 ? 0 : 1;
}
