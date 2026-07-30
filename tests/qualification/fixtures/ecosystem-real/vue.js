function main() {
  const vue = require("vue");
  const count = vue.ref(2);
  const doubled = vue.computed(() => count.value * 2);
  console.log("vue-ok", doubled.value);
  return doubled.value === 4 ? 0 : 1;
}
