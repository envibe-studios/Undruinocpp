/**
 * Generate continuous graybox expedition terrain mesh (OBJ) + r16 heightmap.
 */
const fs = require("fs");
const path = require("path");

const X_MIN = -12000,
  X_MAX = 12000;
const Y_MIN = -2000,
  Y_MAX = 17000;
const STEP = 100;

const POIS = [
  [0, 1500, 0, 4500, 1.2],
  [-8000, 3000, -250, 2800, 1.1],
  [8000, 3000, 350, 2800, 1.1],
  [0, 9000, 450, 2800, 1.15],
  [0, 14200, 650, 3200, 1.2],
];

const ROADS = [
  [
    [0, 3000],
    [-8000, 3000],
  ],
  [
    [0, 3000],
    [8000, 3000],
  ],
  [
    [0, 3000],
    [0, 9000],
  ],
  [
    [0, 9000],
    [0, 14200],
  ],
];
const ROAD_HALF_WIDTH = 700;
const ROAD_BLEND = 350;
const ROAD_RAISE = 25;

function clamp(v, lo, hi) {
  return v < lo ? lo : v > hi ? hi : v;
}
function smoothstep(e0, e1, x) {
  const t = clamp((x - e0) / (e1 - e0), 0, 1);
  return t * t * (3 - 2 * t);
}

function poiHeight(x, y) {
  const base = 40 * Math.sin(x * 0.00035) * Math.cos(y * 0.00028);
  let num = 0,
    den = 0;
  for (const [px, py, h, radius, soft] of POIS) {
    const dx = x - px,
      dy = y - py;
    const d = Math.sqrt(dx * dx + dy * dy);
    const w = 1 / (1 + Math.pow(d / Math.max(radius, 1), 2 * soft));
    num += w * h;
    den += w;
  }
  return base + (den > 1e-6 ? num / den : 0);
}

function distToSegment(px, py, ax, ay, bx, by) {
  const abx = bx - ax,
    aby = by - ay;
  const apx = px - ax,
    apy = py - ay;
  const ab2 = abx * abx + aby * aby;
  if (ab2 < 1e-6) return Math.sqrt(apx * apx + apy * apy);
  const t = clamp((apx * abx + apy * aby) / ab2, 0, 1);
  const cx = ax + t * abx,
    cy = ay + t * aby;
  const dx = px - cx,
    dy = py - cy;
  return Math.sqrt(dx * dx + dy * dy);
}

function roadInfluence(x, y) {
  let bestD = 1e18,
    bestH = 0;
  for (const poly of ROADS) {
    for (let i = 0; i < poly.length - 1; i++) {
      const [ax, ay] = poly[i];
      const [bx, by] = poly[i + 1];
      const d = distToSegment(x, y, ax, ay, bx, by);
      if (d < bestD) {
        bestD = d;
        const abx = bx - ax,
          aby = by - ay;
        const ab2 = abx * abx + aby * aby;
        const apx = x - ax,
          apy = y - ay;
        const t = ab2 < 1e-6 ? 0 : clamp((apx * abx + apy * aby) / ab2, 0, 1);
        const ha = poiHeight(ax, ay);
        const hb = poiHeight(bx, by);
        bestH = ha + (hb - ha) * t + ROAD_RAISE;
      }
    }
  }
  const outer = ROAD_HALF_WIDTH + ROAD_BLEND;
  if (bestD >= outer) return [0, bestH];
  if (bestD <= ROAD_HALF_WIDTH) return [1, bestH];
  return [1 - smoothstep(ROAD_HALF_WIDTH, outer, bestD), bestH];
}

function sampleHeight(x, y) {
  const h = poiHeight(x, y);
  const [infl, roadH] = roadInfluence(x, y);
  if (infl <= 0) return h;
  return h * (1 - infl) + roadH * infl;
}

const xs = Math.floor((X_MAX - X_MIN) / STEP) + 1;
const ys = Math.floor((Y_MAX - Y_MIN) / STEP) + 1;
const verts = new Array(xs * ys);
let zmin = Infinity,
  zmax = -Infinity;

for (let j = 0; j < ys; j++) {
  const y = Y_MIN + j * STEP;
  for (let i = 0; i < xs; i++) {
    const x = X_MIN + i * STEP;
    const z = sampleHeight(x, y);
    verts[j * xs + i] = [x, y, z];
    if (z < zmin) zmin = z;
    if (z > zmax) zmax = z;
  }
}

const outDir = path.join(__dirname, "..", "Content", "Level", "Graybox");
fs.mkdirSync(outDir, { recursive: true });
const objPath = path.join(outDir, "SM_GrayboxExpeditionTerrain.obj");

let obj = `# GrayboxExpedition continuous terrain\n# grid ${xs} x ${ys}, step ${STEP}cm\n`;
for (const [x, y, z] of verts) {
  obj += `v ${x.toFixed(3)} ${y.toFixed(3)} ${z.toFixed(3)}\n`;
}
for (let j = 0; j < ys - 1; j++) {
  for (let i = 0; i < xs - 1; i++) {
    const i0 = j * xs + i + 1;
    const i1 = i0 + 1;
    const i2 = i0 + xs;
    const i3 = i2 + 1;
    obj += `f ${i0} ${i1} ${i3}\n`;
    obj += `f ${i0} ${i3} ${i2}\n`;
  }
}
fs.writeFileSync(objPath, obj);

const r16Path = path.join(outDir, "SM_GrayboxExpeditionTerrain.r16");
const zrange = Math.max(zmax - zmin, 1);
const buf = Buffer.alloc(xs * ys * 2);
for (let n = 0; n < verts.length; n++) {
  const u16 = Math.round(clamp((verts[n][2] - zmin) / zrange, 0, 1) * 65535);
  buf.writeUInt16LE(u16, n * 2);
}
fs.writeFileSync(r16Path, buf);

fs.writeFileSync(
  path.join(outDir, "SM_GrayboxExpeditionTerrain.json"),
  JSON.stringify(
    {
      xs,
      ys,
      step_cm: STEP,
      x_min: X_MIN,
      x_max: X_MAX,
      y_min: Y_MIN,
      y_max: Y_MAX,
      z_min: zmin,
      z_max: zmax,
      obj: objPath,
      r16: r16Path,
    },
    null,
    2
  )
);

console.log(
  `Wrote ${objPath} (${xs}x${ys} verts=${verts.length}) z=[${zmin.toFixed(1)},${zmax.toFixed(1)}]`
);
