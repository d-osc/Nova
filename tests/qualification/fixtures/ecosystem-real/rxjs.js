function main() {
  const rxjs = require("rxjs");
  let total = 0;
  rxjs.of(1, 2, 3).subscribe((value) => total += value);
  console.log("rxjs-ok", total);
  return total === 6 ? 0 : 1;
}
