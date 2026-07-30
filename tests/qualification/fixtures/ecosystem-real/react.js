function main() {
  const React = require("react");
  const element = React.createElement("div", null, "Nova");
  console.log("react-ok", element.type);
  return element.type === "div" ? 0 : 1;
}
