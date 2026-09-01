"""Generate a continuous graybox expedition terrain mesh (OBJ) with POI height variation and road corridors."""
from __future__ import annotations

import math
import struct
from pathlib import Path

# World coverage (cm), matching graybox POI ring
X_MIN, X_MAX = -12000.0, 12000.0
Y_MIN, Y_MAX = -2000.0, 17000.0
# ~100 cm spacing
STEP = 100.0

# POI height targets (surface Z cm)
POIS = [
    # (x, y, height, radius, falloff softness)
    (0.0, 1500.0, 0.0, 4500.0, 1.2),       # spawn/hub basin
    (-8000.0, 3000.0, -250.0, 2800.0, 1.1),  # scan valley
    (8000.0, 3000.0, 350.0, 2800.0, 1.1),    # harvest rise
    (0.0, 9000.0, 450.0, 2800.0, 1.15),      # ambush ridge
    (0.0, 14200.0, 650.0, 3200.0, 1.2),      # base plateau
]

# Road polylines: list of (x,y) waypoints; carved flatter/slightly raised path
ROADS = [
    [(0.0, 3000.0), (-8000.0, 3000.0)],          # hub -> scan
    [(0.0, 3000.0), (8000.0, 3000.0)],           # hub -> harvest
    [(0.0, 3000.0), (0.0, 9000.0)],              # hub -> ambush
    [(0.0, 9000.0), (0.0, 14200.0)],             # ambush -> base
]
ROAD_HALF_WIDTH = 700.0
ROAD_BLEND = 350.0
ROAD_RAISE = 25.0


def clamp(v: float, lo: float, hi: float) -> float:
    return lo if v < lo else hi if v > hi else v


def smoothstep(edge0: float, edge1: float, x: float) -> float:
    t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def poi_height(x: float, y: float) -> float:
    # Weighted blend of POI targets over a mild base undulation
    base = 40.0 * math.sin(x * 0.00035) * math.cos(y * 0.00028)
    num = 0.0
    den = 0.0
    for px, py, h, radius, soft in POIS:
        dx = x - px
        dy = y - py
        d = math.sqrt(dx * dx + dy * dy)
        # Inverse-distance weight with soft falloff outside radius
        w = 1.0 / (1.0 + (d / max(radius, 1.0)) ** (2.0 * soft))
        num += w * h
        den += w
    return base + (num / den if den > 1e-6 else 0.0)


def dist_to_segment(px: float, py: float, ax: float, ay: float, bx: float, by: float) -> float:
    abx, aby = bx - ax, by - ay
    apx, apy = px - ax, py - ay
    ab2 = abx * abx + aby * aby
    if ab2 < 1e-6:
        return math.sqrt(apx * apx + apy * apy)
    t = clamp((apx * abx + apy * aby) / ab2, 0.0, 1.0)
    cx, cy = ax + t * abx, ay + t * aby
    dx, dy = px - cx, py - cy
    return math.sqrt(dx * dx + dy * dy)


def road_influence(x: float, y: float) -> tuple[float, float]:
    """Return (influence 0..1, target height along road)."""
    best_d = 1e18
    best_h = 0.0
    for poly in ROADS:
        for i in range(len(poly) - 1):
            ax, ay = poly[i]
            bx, by = poly[i + 1]
            d = dist_to_segment(x, y, ax, ay, bx, by)
            if d < best_d:
                best_d = d
                # interpolate height targets at endpoints via poi_height
                # use distance along segment for blend
                abx, aby = bx - ax, by - ay
                ab2 = abx * abx + aby * aby
                apx, apy = x - ax, y - ay
                t = 0.0 if ab2 < 1e-6 else clamp((apx * abx + apy * aby) / ab2, 0.0, 1.0)
                ha = poi_height(ax, ay)
                hb = poi_height(bx, by)
                best_h = ha + (hb - ha) * t + ROAD_RAISE
    outer = ROAD_HALF_WIDTH + ROAD_BLEND
    if best_d >= outer:
        return 0.0, best_h
    if best_d <= ROAD_HALF_WIDTH:
        return 1.0, best_h
    # blend zone
    u = 1.0 - smoothstep(ROAD_HALF_WIDTH, outer, best_d)
    return u, best_h


def sample_height(x: float, y: float) -> float:
    h = poi_height(x, y)
    infl, road_h = road_influence(x, y)
    if infl <= 0.0:
        return h
    # Flatten toward road corridor height
    return h * (1.0 - infl) + road_h * infl


def main() -> None:
    xs = int((X_MAX - X_MIN) / STEP) + 1
    ys = int((Y_MAX - Y_MIN) / STEP) + 1
    out_path = Path(__file__).resolve().parents[1] / "Content" / "Level" / "Graybox" / "SM_GrayboxExpeditionTerrain.obj"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    verts: list[tuple[float, float, float]] = []
    for j in range(ys):
        y = Y_MIN + j * STEP
        for i in range(xs):
            x = X_MIN + i * STEP
            z = sample_height(x, y)
            verts.append((x, y, z))

    # Write OBJ (Unreal: X forward? Import typically Y-up or Z-up depending on options.
    # UE FBX/OBJ import defaults often treat Z-up. We'll emit Z-up: x,y horizontal, z height.
    with out_path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# GrayboxExpedition continuous terrain\n")
        f.write(f"# grid {xs} x {ys}, step {STEP}cm\n")
        for x, y, z in verts:
            f.write(f"v {x:.3f} {y:.3f} {z:.3f}\n")
        # faces (1-based), CCW when viewed from above
        for j in range(ys - 1):
            for i in range(xs - 1):
                i0 = j * xs + i + 1
                i1 = i0 + 1
                i2 = i0 + xs
                i3 = i2 + 1
                f.write(f"f {i0} {i1} {i3}\n")
                f.write(f"f {i0} {i3} {i2}\n")

    # Also write a 16-bit raw heightmap for optional Landscape import by hand
    hm_path = out_path.with_suffix(".r16")
    zmin = min(v[2] for v in verts)
    zmax = max(v[2] for v in verts)
    zrange = max(zmax - zmin, 1.0)
    with hm_path.open("wb") as f:
        for j in range(ys):
            for i in range(xs):
                z = verts[j * xs + i][2]
                u16 = int(clamp((z - zmin) / zrange, 0.0, 1.0) * 65535.0 + 0.5)
                f.write(struct.pack("<H", u16))

    meta = out_path.with_suffix(".json")
    meta.write_text(
        "{\n"
        f'  "xs": {xs},\n'
        f'  "ys": {ys},\n'
        f'  "step_cm": {STEP},\n'
        f'  "x_min": {X_MIN}, "x_max": {X_MAX},\n'
        f'  "y_min": {Y_MIN}, "y_max": {Y_MAX},\n'
        f'  "z_min": {zmin:.3f}, "z_max": {zmax:.3f},\n'
        f'  "obj": "{out_path.as_posix()}",\n'
        f'  "r16": "{hm_path.as_posix()}"\n'
        "}\n",
        encoding="utf-8",
    )
    print(f"Wrote {out_path} ({xs}x{ys} verts={len(verts)}) z=[{zmin:.1f},{zmax:.1f}]")
    print(f"Wrote {hm_path}")


if __name__ == "__main__":
    main()
