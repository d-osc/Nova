function main() {
  const React = require("react");
  const server = require("react-dom/server");
  const html = server.renderToString(React.createElement("span", null, "Nova"));
  console.log("react-dom-ok", html);
  return html.length > 0 ? 0 : 1;
}
