const fs = require("fs");
const t = fs.readFileSync(
  "C:/Users/mattes/.cursor/projects/d-Unreals-Undruinocpp/agent-tools/cf862e0d-a454-42bb-be97-b40af256b934.txt",
  "utf8"
);
const m = [...t.matchAll(/BlueprintTools\\.([a-zA-Z0-9_]+)/g)].map((x) => x[1]);
console.log([...new Set(m)].sort().join("\n"));
