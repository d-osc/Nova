function main() {
  const express = require("express");
  const app = express();
  app.get("/health", (_request, response) => response.send("ok"));
  console.log("express-ok");
  return 0;
}
