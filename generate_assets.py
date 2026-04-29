#!/usr/bin/env python3
"""
generate_assets.py
Generates two PNG assets for the Couch LA-2A plugin:
  1. Resources/knob_filmstrip.png  — 100-frame rotary knob filmstrip (150×15000 px)
  2. Resources/panel_metal.png     — brushed aluminium panel texture  (700×210 px)
"""

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import math, os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RES_DIR    = os.path.join(SCRIPT_DIR, "Resources")
os.makedirs(RES_DIR, exist_ok=True)

# ─────────────────────────────────────────────────────────────────────────────
# 1. BRUSHED METAL PANEL TEXTURE
# ─────────────────────────────────────────────────────────────────────────────

def generate_panel_metal(w=700, h=210):
    print("Generating panel_metal.png ...")
    rng = np.random.default_rng(42)

    # ── Base gradient: light at top-center, darker at bottom corners ─────────
    y_arr, x_arr = np.mgrid[0:h, 0:w]
    # Normalised radial distance from upper-centre hotspot
    hx, hy = w * 0.42, h * 0.28
    rad = np.sqrt(((x_arr - hx) / (w * 0.65))**2 + ((y_arr - hy) / (h * 0.55))**2)
    base = np.clip(1.0 - rad * 0.48, 0.72, 1.0)

    # Secondary soft highlight: lower-right  
    hx2, hy2 = w * 0.78, h * 0.68
    rad2 = np.sqrt(((x_arr - hx2) / (w * 0.55))**2 + ((y_arr - hy2) / (h * 0.5))**2)
    base += np.clip(0.07 - rad2 * 0.12, 0.0, 0.07)

    # ── Horizontal brush strokes ──────────────────────────────────────────────
    # Each row gets a random brightness perturbation; long streaks vary slowly
    streak = np.zeros(h, dtype=np.float32)
    val = 0.0
    for row in range(h):
        val += rng.normal(0, 0.008)
        val  = np.clip(val, -0.06, 0.06)
        streak[row] = val
    base += streak[:, np.newaxis]

    # Fine horizontal noise (individual scratch lines)
    for _ in range(320):
        row   = rng.integers(0, h)
        x0    = rng.integers(0, w // 2)
        x1    = rng.integers(w // 2, w)
        alpha = rng.uniform(0.012, 0.055)
        sign  = rng.choice([-1, 1])
        base[row, x0:x1] += sign * alpha

    # Very sparse bright scratch lines
    for _ in range(18):
        row   = rng.integers(0, h)
        x0    = rng.integers(0, w // 3)
        x1    = rng.integers(2 * w // 3, w)
        base[row, x0:x1] = np.clip(base[row, x0:x1] + rng.uniform(0.07, 0.14), 0, 1)

    # ── Vertical micro-variation (simulates cross-grain reflection bands) ─────
    col_mod = np.sin(np.linspace(0, np.pi * 3.5, w)) * 0.018
    base += col_mod[np.newaxis, :]

    base = np.clip(base, 0.0, 1.0)

    # ── Convert to RGB image ──────────────────────────────────────────────────
    # Slight warm-neutral tint: R=0.97 G=0.98 B=1.00
    r = np.clip(base * 0.97 * 255, 0, 255).astype(np.uint8)
    g = np.clip(base * 0.98 * 255, 0, 255).astype(np.uint8)
    b = np.clip(base * 1.00 * 255, 0, 255).astype(np.uint8)

    rgb = np.stack([r, g, b], axis=2)
    img = Image.fromarray(rgb, 'RGB')

    # Subtle gaussian blur to blend strokes (keep crisp streaks but soften noise)
    img = img.filter(ImageFilter.GaussianBlur(0.5))

    out = os.path.join(RES_DIR, "panel_metal.png")
    img.save(out, "PNG")
    print(f"  Saved {out}  ({w}×{h})")


# ─────────────────────────────────────────────────────────────────────────────
# 2. KNOB FILMSTRIP
# ─────────────────────────────────────────────────────────────────────────────

FRAME_W   = 150
FRAME_H   = 150
N_FRAMES  = 100
START_DEG = -135.0
END_DEG   =  135.0

# Supersampling factor — render at 3× then downsample
SS = 3


def draw_knob_frame(angle_deg: float) -> Image.Image:
    W  = FRAME_W * SS
    H  = FRAME_H * SS
    cx = W / 2.0
    cy = H / 2.0

    angle_rad = math.radians(angle_deg)

    # Radii (in supersampled pixels)
    R_SHADOW  = W * 0.470
    R_SKIRT   = W * 0.455   # outer edge of skirt/flange
    R_GROOVE  = W * 0.385   # inner edge of skirt (start of body cylinder)
    R_BEVEL   = W * 0.358   # chrome bevel ring
    R_CAP     = W * 0.268   # brushed-aluminium top cap
    R_HUB     = W * 0.072   # centre hub cap

    y_arr, x_arr = np.mgrid[0:H, 0:W].astype(np.float32)
    dx = x_arr - cx
    dy = y_arr - cy
    dist = np.sqrt(dx * dx + dy * dy)
    # Angle from 12-o'clock, clockwise
    theta = np.arctan2(dx, -dy)   # range [−π, π]

    canvas = np.zeros((H, W, 4), dtype=np.float32)

    # ─── Helper: set pixels inside mask ──────────────────────────────────────
    def set_px(mask, r, g, b, a=1.0):
        canvas[..., 0] = np.where(mask, r, canvas[..., 0])
        canvas[..., 1] = np.where(mask, g, canvas[..., 1])
        canvas[..., 2] = np.where(mask, b, canvas[..., 2])
        canvas[..., 3] = np.where(mask, a, canvas[..., 3])

    def blend_px(mask, r, g, b, alpha):
        """Alpha-blend over existing canvas."""
        a_src = np.where(mask, alpha, 0.0)
        canvas[..., 0] = canvas[..., 0] * (1 - a_src) + r * a_src
        canvas[..., 1] = canvas[..., 1] * (1 - a_src) + g * a_src
        canvas[..., 2] = canvas[..., 2] * (1 - a_src) + b * a_src
        canvas[..., 3] = np.where(mask, np.maximum(canvas[..., 3], alpha), canvas[..., 3])

    # ── 1. Outer skirt / flange ───────────────────────────────────────────────
    # The skirt is the conical flared base: dark anodised aluminium.
    # Visible as a ring between R_SKIRT and R_GROOVE when viewed from above.
    skirt_mask = (dist <= R_SKIRT) & (dist > R_GROOVE)

    # Normalised outward direction
    dnx = np.where(dist > 0, dx / dist, 0.0)
    dny = np.where(dist > 0, dy / dist, 0.0)

    # Diffuse lighting on conical surface — light from upper-left
    LX, LY = -0.55, -0.70
    skirt_diff = np.clip(dnx * LX + dny * LY, -1, 1) * 0.5 + 0.5

    # Fade from skirt outer (darker) to inner edge (slightly lighter)
    skirt_t = np.clip((dist - R_GROOVE) / (R_SKIRT - R_GROOVE), 0, 1)

    skirt_base = 0.07 + skirt_diff * 0.16 + (1.0 - skirt_t) * 0.06
    # Edge specular glint on upper-left rim
    edge_spec_dir = np.clip(dnx * (-0.7) + dny * (-0.72), 0, 1)
    skirt_base += edge_spec_dir ** 6 * skirt_t * 0.55

    sb = np.clip(skirt_base, 0, 1)
    set_px(skirt_mask, sb * 0.92, sb * 0.93, sb * 0.96)

    # ── 2. Body cylinder (between groove and bevel) ────────────────────────────
    body_mask = (dist <= R_GROOVE) & (dist > R_BEVEL + W * 0.012)

    body_diff = np.clip(dnx * LX + dny * LY, -1, 1) * 0.5 + 0.5
    body_base = 0.05 + body_diff * 0.10
    # Subtle left-edge highlight band
    body_edge_l = np.clip((-dnx * 0.85 + (-dny) * 0.52), 0, 1) ** 4 * 0.35
    body_base += body_edge_l

    bb = np.clip(body_base, 0, 1)
    set_px(body_mask, bb * 0.90, bb * 0.91, bb * 0.94)

    # ── 3. Chrome bevel ring ───────────────────────────────────────────────────
    bevel_half = W * 0.014
    bevel_mask = np.abs(dist - R_BEVEL) < bevel_half
    bevel_t    = 1.0 - np.abs(dist - R_BEVEL) / bevel_half

    # Chrome: bright on upper-left, dark lower-right
    chr_diff  = np.clip(dnx * (-0.6) + dny * (-0.78), 0, 1)
    chr_spec  = chr_diff ** 5
    chr_val   = np.clip(0.40 + chr_diff * 0.35 + chr_spec * 0.28, 0, 1)

    blend_px(bevel_mask, chr_val, chr_val, chr_val, bevel_t)

    # ── 4. Brushed-aluminium top cap ───────────────────────────────────────────
    cap_mask = dist <= R_CAP

    cap_dx = dx / (R_CAP + 1e-9)
    cap_dy = dy / (R_CAP + 1e-9)
    cap_d2 = np.clip(cap_dx**2 + cap_dy**2, 0, 1.0)
    cap_nz = np.sqrt(1.0 - cap_d2)

    # Dome diffuse
    cap_diff = np.clip(cap_dx * (-0.50) + cap_dy * (-0.65) + cap_nz * 0.60, 0, 1)

    # Blinn-Phong specular
    HX, HY, HZ = -0.50, -0.65, 1.60
    Hn = math.sqrt(HX*HX + HY*HY + HZ*HZ)
    HX, HY, HZ = HX/Hn, HY/Hn, HZ/Hn
    cap_spec = np.clip(cap_dx * HX + cap_dy * HY + cap_nz * HZ, 0, 1) ** 40

    # Concentric machined ring texture
    ring_t = (dist % (W * 0.022)) / (W * 0.022)
    ring_tex = 0.5 + ring_t * 0.14

    # Radial brush lines (very subtle)
    radial_tex = (np.sin(theta * 80) * 0.5 + 0.5) * 0.025

    cap_val = np.clip(0.68 + cap_diff * 0.18 + ring_tex * 0.06
                      + radial_tex - cap_d2 * 0.08 + cap_spec * 0.30, 0, 1)

    set_px(cap_mask, cap_val * 0.96, cap_val * 0.97, cap_val * 1.00)

    # ── 5. Indicator stripe (white, on body + cap groove) ─────────────────────
    sa = math.sin(angle_rad)
    ca = math.cos(angle_rad)   # clockface: 12=up

    # Perpendicular / parallel projections
    dot_perp = dx * ca  + dy * sa          # perpendicular distance from line
    dot_para = dx * sa  + dy * (-ca)       # parallel distance along line

    stripe_w    = W * 0.024
    stripe_soft = stripe_w * 1.5
    stripe_fade = np.clip(1.0 - np.abs(dot_perp) / stripe_soft, 0, 1)
    stripe_on   = (dot_para >= -R_HUB * 0.2) & (dot_para <= R_BEVEL * 0.97)

    # On body ring: bright white stripe
    body_stripe = stripe_on & (dist >= R_CAP * 1.02) & (dist <= R_BEVEL * 0.99)
    sv = np.clip(0.82 + stripe_fade * 0.18, 0, 1)
    blend_px(body_stripe, sv, sv, sv, stripe_fade * 0.98)

    # On cap: dark groove line
    cap_groove = stripe_on & cap_mask & (dot_para > R_HUB * 0.5)
    cap_groove_fade = np.clip(1.0 - np.abs(dot_perp) / (stripe_w * 0.8), 0, 1)
    groove_darken = canvas[..., 0] - cap_groove_fade * 0.32
    canvas[..., 0] = np.where(cap_groove, groove_darken, canvas[..., 0])
    canvas[..., 1] = np.where(cap_groove, groove_darken, canvas[..., 1])
    canvas[..., 2] = np.where(cap_groove, groove_darken, canvas[..., 2])

    # ── 6. Centre hub ─────────────────────────────────────────────────────────
    hub_mask = dist <= R_HUB
    hub_dx = dx / (R_HUB + 1e-9)
    hub_dy = dy / (R_HUB + 1e-9)
    hub_d2 = np.clip(hub_dx**2 + hub_dy**2, 0, 1.0)
    hub_nz = np.sqrt(1.0 - hub_d2)
    hub_diff = np.clip(hub_dx * (-0.5) + hub_dy * (-0.65) + hub_nz * 0.7, 0, 1)
    hub_spec = np.clip(hub_dx * HX + hub_dy * HY + hub_nz * HZ, 0, 1) ** 28
    hub_val  = np.clip(0.52 + hub_diff * 0.30 + hub_spec * 0.22, 0, 1)
    set_px(hub_mask, hub_val, hub_val, hub_val)
    # Thin dark ring around hub
    hub_ring = (dist <= R_HUB * 1.18) & (dist > R_HUB)
    set_px(hub_ring, 0.08, 0.08, 0.09)

    # ── 7. Convert canvas → PIL, add shadow, downsample ───────────────────────
    arr_u8 = (np.clip(canvas, 0, 1) * 255).astype(np.uint8)
    hi_res = Image.fromarray(arr_u8, 'RGBA')

    # Drop shadow at high-res then composite
    shadow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    sdraw  = ImageDraw.Draw(shadow)
    sox, soy = int(W * 0.025), int(W * 0.030)
    sr = int(R_SKIRT * 0.97)
    sdraw.ellipse([int(cx) - sr + sox, int(cy) - sr + soy,
                   int(cx) + sr + sox, int(cy) + sr + soy],
                  fill=(0, 0, 0, 110))
    shadow = shadow.filter(ImageFilter.GaussianBlur(SS * 4.5))

    frame_bg = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    frame_bg = Image.alpha_composite(frame_bg, shadow)
    frame_bg = Image.alpha_composite(frame_bg, hi_res)

    # Downsample with LANCZOS for smooth antialiasing
    return frame_bg.resize((FRAME_W, FRAME_H), Image.LANCZOS)


def generate_filmstrip():
    print(f"Generating knob_filmstrip.png ({N_FRAMES} frames × {FRAME_W}×{FRAME_H}px)...")
    filmstrip = Image.new('RGBA', (FRAME_W, FRAME_H * N_FRAMES), (0, 0, 0, 0))

    for i in range(N_FRAMES):
        t     = i / (N_FRAMES - 1)
        angle = START_DEG + t * (END_DEG - START_DEG)
        frame = draw_knob_frame(angle)
        filmstrip.paste(frame, (0, i * FRAME_H))
        if i % 10 == 0:
            print(f"  frame {i:3d}/{N_FRAMES}  angle={angle:+.1f}°")

    out = os.path.join(RES_DIR, "knob_filmstrip.png")
    filmstrip.save(out, "PNG")
    print(f"  Saved {out}  ({FRAME_W}×{FRAME_H * N_FRAMES})")


# ─────────────────────────────────────────────────────────────────────────────
# 3. TOGGLE SWITCH  (two states: up = LIMIT, down = COMPRESS)
# ─────────────────────────────────────────────────────────────────────────────
#
# Front-view of a classic metal bat-toggle:
#   ┌──────────────────────────┐
#   │   ╔═══╗   <- dome tip    │  <- housing (dark recessed surround)
#   │   ║   ║   <- shaft       │
#   │  [●●●●●] <- pivot collar │
#   │   ║   ║                  │
#   │   ╚═══╝   <- base        │
#   └──────────────────────────┘
#
# Output: toggle_up.png  (LIMIT active  — bat points upward)
#         toggle_down.png (COMPRESS active — bat points downward)
#  Each image: TW × TH pixels (RGBA)

TW = 72   # toggle image width  (matches the clickable area in the plugin)
TH = 90   # toggle image height (spans both mode label areas)
TSS = 4   # supersampling factor

def draw_toggle(bat_up: bool) -> Image.Image:
    W  = TW * TSS
    H  = TH * TSS
    cx = W / 2.0

    canvas = np.zeros((H, W, 4), dtype=np.float32)

    y_arr, x_arr = np.mgrid[0:H, 0:W].astype(np.float32)

    # ── Geometry ──────────────────────────────────────────────────────────
    housing_pad  = W * 0.08
    housing_x0   = housing_pad
    housing_x1   = W - housing_pad
    housing_y0   = H * 0.04
    housing_y1   = H * 0.96
    housing_rx   = (housing_x1 - housing_x0) * 0.22   # corner radius

    shaft_w      = W * 0.20
    shaft_half   = shaft_w / 2.0
    pivot_r      = W * 0.175          # collar radius
    pivot_cy     = H * 0.50           # collar always at vertical centre
    dome_r       = W * 0.155          # tip dome radius
    dome_cy      = H * 0.16 if bat_up else H * 0.84   # tip centre
    shaft_y0     = min(pivot_cy - pivot_r * 0.6, dome_cy + dome_r * 0.7) if bat_up \
                   else pivot_cy + pivot_r * 0.6
    shaft_y1     = dome_cy - dome_r * 0.65 if bat_up \
                   else max(pivot_cy + pivot_r * 0.6, dome_cy - dome_r * 0.7)

    # ── Helper: set / blend pixels ─────────────────────────────────────────
    def set_px(mask, r, g, b, a=1.0):
        canvas[..., 0] = np.where(mask, r, canvas[..., 0])
        canvas[..., 1] = np.where(mask, g, canvas[..., 1])
        canvas[..., 2] = np.where(mask, b, canvas[..., 2])
        canvas[..., 3] = np.where(mask, a, canvas[..., 3])

    def blend_px(mask, r, g, b, alpha):
        a = np.where(mask, alpha, 0.0)
        canvas[..., 0] = canvas[..., 0] * (1 - a) + r * a
        canvas[..., 1] = canvas[..., 1] * (1 - a) + g * a
        canvas[..., 2] = canvas[..., 2] * (1 - a) + b * a
        canvas[..., 3] = np.where(mask, np.maximum(canvas[..., 3], alpha), canvas[..., 3])

    # ── 1. Housing (rounded-rect, dark gunmetal) ──────────────────────────
    # Approximate rounded rect with numpy
    in_rect   = (x_arr >= housing_x0) & (x_arr <= housing_x1) \
              & (y_arr >= housing_y0) & (y_arr <= housing_y1)
    # Rounded corners: exclude corners outside the arc
    def corner_ok(px, py, ax, ay):
        return ((px - ax)**2 + (py - ay)**2) <= housing_rx**2
    corners_inside = (
          np.where((x_arr < housing_x0 + housing_rx) & (y_arr < housing_y0 + housing_rx),
                   corner_ok(x_arr, y_arr, housing_x0 + housing_rx, housing_y0 + housing_rx), True)
        & np.where((x_arr > housing_x1 - housing_rx) & (y_arr < housing_y0 + housing_rx),
                   corner_ok(x_arr, y_arr, housing_x1 - housing_rx, housing_y0 + housing_rx), True)
        & np.where((x_arr < housing_x0 + housing_rx) & (y_arr > housing_y1 - housing_rx),
                   corner_ok(x_arr, y_arr, housing_x0 + housing_rx, housing_y1 - housing_rx), True)
        & np.where((x_arr > housing_x1 - housing_rx) & (y_arr > housing_y1 - housing_rx),
                   corner_ok(x_arr, y_arr, housing_x1 - housing_rx, housing_y1 - housing_rx), True)
    )
    housing_mask = in_rect & corners_inside

    # Housing gradient: slightly lighter top-left
    h_bright = 0.18 + (1.0 - y_arr / H) * 0.10 + (1.0 - x_arr / W) * 0.06
    set_px(housing_mask, h_bright * 0.88, h_bright * 0.89, h_bright * 0.92)

    # Housing inner-bevel highlight (top & left inner edge)
    bevel_t  = 3.0 * TSS
    top_bevel  = housing_mask & (y_arr < housing_y0 + bevel_t)
    left_bevel = housing_mask & (x_arr < housing_x0 + bevel_t)
    blend_px(top_bevel | left_bevel, 0.55, 0.56, 0.58, 0.45)

    # Housing inner-bevel shadow (bottom & right inner edge)
    bot_bevel   = housing_mask & (y_arr > housing_y1 - bevel_t)
    right_bevel = housing_mask & (x_arr > housing_x1 - bevel_t)
    blend_px(bot_bevel | right_bevel, 0.05, 0.05, 0.06, 0.55)

    # ── 2. Inner recess (oval cavity the bat comes out of) ────────────────
    recess_rx = W * 0.22
    recess_ry = H * 0.26
    recess_dist = ((x_arr - cx) / recess_rx)**2 + ((y_arr - pivot_cy) / recess_ry)**2
    recess_mask = (recess_dist <= 1.0) & housing_mask

    recess_v = np.clip(0.04 + recess_dist * 0.10, 0, 0.14)
    set_px(recess_mask, recess_v * 0.85, recess_v * 0.86, recess_v * 0.90)

    # Recess inner-shadow ring
    recess_ring = (recess_dist >= 0.72) & (recess_dist <= 1.0) & housing_mask
    ring_alpha  = np.clip((recess_dist - 0.72) / 0.28, 0, 1) * 0.6
    blend_px(recess_ring, 0, 0, 0, ring_alpha)

    # ── 3. Shaft (rectangular pill) ───────────────────────────────────────
    shaft_mask = (np.abs(x_arr - cx) <= shaft_half) \
               & (y_arr >= min(shaft_y0, shaft_y1)) \
               & (y_arr <= max(shaft_y0, shaft_y1))

    shaft_dx   = (x_arr - cx) / shaft_half   # -1..+1
    # Cylindrical shading: bright centre-left, dark right edge
    cyl_diff   = np.clip(1.0 - shaft_dx**2, 0, 1) ** 0.55
    shaft_v    = np.clip(0.38 + cyl_diff * 0.38, 0, 1)
    # Specular stripe: left of centre
    shaft_spec = np.clip((0.35 - shaft_dx) * 3.5, 0, 1) ** 4 * 0.45
    shaft_v    = np.clip(shaft_v + shaft_spec, 0, 1)

    set_px(shaft_mask, shaft_v * 0.93, shaft_v * 0.94, shaft_v * 0.97)

    # ── 4. Pivot collar ────────────────────────────────────────────────────
    collar_dist = np.sqrt((x_arr - cx)**2 + (y_arr - pivot_cy)**2)
    collar_mask = collar_dist <= pivot_r

    col_dx = (x_arr - cx)    / (pivot_r + 1e-9)
    col_dy = (y_arr - pivot_cy) / (pivot_r + 1e-9)
    col_d2 = np.clip(col_dx**2 + col_dy**2, 0, 1.0)
    col_nz = np.sqrt(1.0 - col_d2)

    col_diff = np.clip(col_dx * (-0.55) + col_dy * (-0.65) + col_nz * 0.55, 0, 1)
    # Blinn-Phong on collar
    HX2, HY2, HZ2 = -0.55, -0.65, 1.5
    Hn2 = (HX2**2 + HY2**2 + HZ2**2) ** 0.5
    HX2, HY2, HZ2 = HX2/Hn2, HY2/Hn2, HZ2/Hn2
    col_spec = np.clip(col_dx * HX2 + col_dy * HY2 + col_nz * HZ2, 0, 1) ** 32
    col_v    = np.clip(0.45 + col_diff * 0.30 + col_spec * 0.28, 0, 1)

    # Concentric ring texture on collar
    ring_tex2 = (collar_dist % (pivot_r * 0.18)) / (pivot_r * 0.18)
    col_v     = np.clip(col_v + ring_tex2 * 0.04, 0, 1)

    set_px(collar_mask, col_v * 0.93, col_v * 0.94, col_v * 0.97)

    # ── 5. Dome tip ────────────────────────────────────────────────────────
    dome_dist = np.sqrt((x_arr - cx)**2 + (y_arr - dome_cy)**2)
    dome_mask = dome_dist <= dome_r

    dome_dx = (x_arr - cx)     / (dome_r + 1e-9)
    dome_dy = (y_arr - dome_cy) / (dome_r + 1e-9)
    dome_d2 = np.clip(dome_dx**2 + dome_dy**2, 0, 1.0)
    dome_nz = np.sqrt(1.0 - dome_d2)

    dome_diff = np.clip(dome_dx * (-0.55) + dome_dy * (-0.65) + dome_nz * 0.60, 0, 1)
    dome_spec = np.clip(dome_dx * HX2 + dome_dy * HY2 + dome_nz * HZ2, 0, 1) ** 40
    dome_v    = np.clip(0.55 + dome_diff * 0.28 + dome_spec * 0.22, 0, 1)

    # Concentric rings on dome cap
    ring_tex3 = (dome_dist % (dome_r * 0.22)) / (dome_r * 0.22)
    dome_v    = np.clip(dome_v + ring_tex3 * 0.05, 0, 1)

    set_px(dome_mask, dome_v * 0.93, dome_v * 0.94, dome_v * 0.97)

    # Dome specular flash (upper-left arc)
    spec_dist_from_flash = np.sqrt((x_arr - (cx - dome_r * 0.3))**2
                                 + (y_arr - (dome_cy - dome_r * 0.35))**2)
    spec_flash = np.clip(1.0 - spec_dist_from_flash / (dome_r * 0.45), 0, 1) ** 2
    blend_px(dome_mask, 1.0, 1.0, 1.0, spec_flash * 0.55)

    # ── 6. Drop shadow ─────────────────────────────────────────────────────
    # Applied at PIL level after conversion

    # ── Convert → PIL, add shadow, downsample ─────────────────────────────
    arr_u8 = (np.clip(canvas, 0, 1) * 255).astype(np.uint8)
    hi_res = Image.fromarray(arr_u8, 'RGBA')

    # Soft drop shadow on the whole housing
    shadow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    sx, sy = int(W * 0.03), int(H * 0.025)
    s_draw.rounded_rectangle(
        [int(housing_x0) + sx, int(housing_y0) + sy,
         int(housing_x1) + sx, int(housing_y1) + sy],
        radius=int(housing_rx), fill=(0, 0, 0, 100))
    shadow = shadow.filter(ImageFilter.GaussianBlur(TSS * 3.5))

    result = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    result = Image.alpha_composite(result, shadow)
    result = Image.alpha_composite(result, hi_res)

    return result.resize((TW, TH), Image.LANCZOS)


def generate_toggles():
    print("Generating toggle_up.png and toggle_down.png ...")
    for bat_up, name in [(True, "toggle_up.png"), (False, "toggle_down.png")]:
        img = draw_toggle(bat_up)
        out = os.path.join(RES_DIR, name)
        img.save(out, "PNG")
        print(f"  Saved {out}  ({TW}×{TH})")


# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    generate_panel_metal()
    generate_filmstrip()
    generate_toggles()
    print("Done.")
