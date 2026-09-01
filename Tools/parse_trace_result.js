const fs = require("fs");
const t = fs.readFileSync(
  "C:/Users/mattes/.cursor/projects/d-Unreals-Undruinocpp/agent-tools/35f3f7cc-ef36-4f38-bb96-e4469aa410d5.txt",
  "utf8"
);
const outer = JSON.parse(t);
const inner = JSON.parse(outer.returnValue);
console.log(
  JSON.stringify(
    {
      convex: inner.convex,
      traces: inner.traces,
      bounds: inner.bounds,
      traceFlag: inner.agg2
        ? JSON.parse(inner.agg2).collisionTraceFlag
        : null,
      convexCount: inner.agg2
        ? JSON.parse(inner.agg2).aggGeom.convexElems.length
        : null,
    },
    null,
    2
  )
);
