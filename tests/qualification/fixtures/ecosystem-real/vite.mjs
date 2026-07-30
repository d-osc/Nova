import { defineConfig } from "vite";

const config = defineConfig({ build: { target: "es2022" } });

function main() {
  console.log("vite-ok", config.build.target);
  return config.build.target === "es2022" ? 0 : 1;
}
