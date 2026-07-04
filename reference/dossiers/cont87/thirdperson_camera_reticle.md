# cont.87 — TS 3rd-person CAMERA + 3rd-person RETICLE (RE dossier)

Target: `reference/client.dll` (Windows MSVC TS client). Decompiles in `decomp/`.
Goal: match three user-verified behavioral gaps — (A) zoom, (B) orbit, (C) reticle lag.

## TL;DR

The single biggest finding: **TS third person is NOT the stock `cam_idealdist`
"distance-behind" orbit that our `cl_dll/view.cpp` implements.** TS rewrote
`V_CalcNormalRefdef` to use an **over-the-shoulder OFFSET camera** driven by a new
cvar **`cl_camtype`** (0–3 presets). The stock `CAM_Think` (in_camera.cpp) is kept
almost verbatim, but its `cam_idealdist` (distance) is **ignored** for the view —
only `cam_ofs` pitch/yaw feed the camera *direction*; the *position* comes from the
preset back/right/up offsets.

Provenance addresses are Ghidra VA = file RVA + 0x10000000. Function RVAs:
`V_CalcRefdef` 0x69b90, `V_CalcNormalRefdef` 0x67890 (the important one),
`CAM_Think` 0x24c80, `CAM_Init`≈0x26e?? (cvar block at 0x26f00–0x271f0),
`CL_CameraOffset` 0x25720, `CL_IsThirdPerson` 0x4fec0, `CAM_ToThirdPerson` 0x4fe00.

---

## A. ZOOM — the distance actually applied

### RE findings (HIGH confidence on the preset numbers)

TS ignores `cam_idealdist`. In `V_CalcNormalRefdef` (0x67890), inside the
`if (CL_IsThirdPerson())` block, it reads **`cl_camtype`** (cvar ptr `DAT_100df414`,
registered name `"cl_camtype"`, **default `"0"`**) and selects a preset of three
offset scalars — `back` (`DAT_10191450`), `right` (`DAT_10191454`), `up`
(`_DAT_10191458`). Disasm at 0x67fb0–0x68033:

| `cl_camtype` | back (fwd axis) | right (side axis) | up (up axis) |
|---|---|---|---|
| **0 (default)** | **-48.0** | **+16.0** | **+12.0** |
| 1 | -48.0 | -16.0 | +12.0 |
| 2 | -90.0 | 0.0 | +12.0 |
| 3 | `-cam_back` | `+cam_right` | `cam_up` |

- `up` for modes 0/1/2 = constant `_DAT_1007d824` = **12.0**.
- mode-3 cvars (registered in CAM_Init): `cam_back` default **"90"**,
  `cam_right` default **"0"**, `cam_up` default **"12"**.
- Byte-proof: back consts `0xc2400000=-48`, `0xc2b40000=-90`; right `0x41800000=+16`,
  `0xc1800000=-16`.

Then it **clamps** (0x68033–0x680b0):
- `back`  clamped to **[-128.0, 24.0]** (`_DAT_10081e68=-128`, `_DAT_1007d748=24`).
- `right` clamped to **[-24.0, 24.0]** (`_DAT_10081e64=-24`, `+24`).
- `up`    clamped to **[-24.0, 24.0]**.

### Dynamic zoom (MED confidence on the gate; HIGH that it exists)

After the clamps, `back` and `up` are **eased over time** (spring, rate 5.0/s ×
frametime, const `_DAT_1007a8d0=5.0`):

- **Dead** (`ref_params.health` = `param_1[0x23]` < 1): `back` eases toward
  **-180.0** (`_DAT_1007d528`). This is the TS death-cam pull-out (our view.cpp
  currently hard-codes -80 for the death cam — see note in C-block).
- **Alive**: if `(localplayer+0x26c & 0x130c30) != 0` **and** `back > -128`, `back`
  eases toward **-92.0** (`_DAT_10081e60`); otherwise the ease term relaxes to 0 and
  `back` stays at the preset base (-48). i.e. under some player-state flag the camera
  pulls from ~48 out to ~92. `up` similarly springs (target ~32 airborne,
  `_DAT_1007a94c=32`).

`localplayer+0x26c` is a player-entity field (likely a buttons/flags word); the
exact `0x130c30` bit-meaning is **not pinned statically** — needs a live read
(watch that dword while the camera pulls out).

### Apply (the origin math)

```
AngleVectors(cam_ofs_angles(pitch,yaw,0)) -> fwd, right, up
vieworg += back*fwd + up*up_vec + right*right_vec      // (0x68110 loop)
```
(`back` is negative = behind; `right +16` = over the RIGHT shoulder; `up +12` = lift.)

### Diff vs ours (`cl_dll/in_camera.cpp` + `view.cpp`)

| | ours | TS |
|---|---|---|
| distance model | `-cam_idealdist(64)*forward`, straight back, no side/up | preset back/right/up offset |
| default steady back | **64** | **48** base, springs to **92** under a gate |
| side offset | 0 | **+16** (right shoulder) |
| up offset | 0 | **+12** |
| selector cvar | `cam_idealdist` | `cl_camtype` (0–3) |

> ⚠ Reconciling with the user's "zoomed OUT more": the TS *base* (48) is actually
> CLOSER than our 64. The "more zoomed out" the user sees is almost certainly the
> **dynamic ease to ~92** (when moving/the gate is active) and/or the up+side offset
> reframing. Confirm with a live read of `DAT_10191450` (back) while playing — see
> the live-trace note. **Do not** just bump a constant; ship the preset + the ease.

### Drop-in (A) — constants for `cl_dll/in_camera.cpp` / `view.cpp`

Register the selector and mode-3 cvars (add to `CAM_Init()`):
```cpp
cvar_t* cl_camtype;   // 3rd-person preset selector, default "0"
cvar_t* cam_back;     // mode-3 back  distance, default "90"
cvar_t* cam_right;    // mode-3 side  offset,   default "0"
cvar_t* cam_up;       // mode-3 up    offset,   default "12"
...
cl_camtype = gEngfuncs.pfnRegisterVariable("cl_camtype", "0", 0);
cam_back   = gEngfuncs.pfnRegisterVariable("cam_back",  "90", 0);
cam_right  = gEngfuncs.pfnRegisterVariable("cam_right",  "0", 0);
cam_up     = gEngfuncs.pfnRegisterVariable("cam_up",    "12", 0);
```
Preset constants (put near the CAM_* defines):
```cpp
#define TSCAM_UP_DEFAULT   12.0f
#define TSCAM_BACK_MIN   (-128.0f)   // clamp lo
#define TSCAM_BACK_MAX     24.0f     // clamp hi
#define TSCAM_SIDE_CLAMP   24.0f     // right/up clamp is [-24,24]
#define TSCAM_EASE_RATE     5.0f     // per-second spring
#define TSCAM_BACK_MOVING (-92.0f)   // gated pull-out target (see live trace)
```

---

## B. ORBIT — how 3rd person is entered + the exact orbit

### Entry (HIGH confidence)

- `CL_IsThirdPerson()` (0x4fec0) is **just** `return cam_thirdperson (DAT_100df390) != 0;`
  (plus our added death/spectator cases — those are ours, keep them).
- Entered by the `thirdperson` console command (toggle) → `CAM_ToThirdPerson`
  (0x4fe00). It seeds `cam_ofs = (viewpitch, viewyaw, 30.0)` (`0x41f00000=30.0`) — our
  in_camera.cpp already matches this (`cam_ofs[2]=CAM_MIN_DIST=30`). The seeded
  distance is cosmetic now since the view ignores it.
- There is **no** always-on gate; TS just doesn't block `thirdperson` in MP (we
  already removed that block). TS binds it to a key by convention.

### The orbit itself (HIGH confidence on geometry, from V_CalcNormalRefdef)

1. `CAM_Think` (0x24c80) maintains `cam_ofs[pitch,yaw]` by **MoveToward-smoothing**
   toward `viewangles + cam_ideal{pitch,yaw}` (defaults 0/0 → directly behind,
   looking the same way), rate `/4` per frame, unless `cam_snapto` (default 0 →
   smoothing ON). Our `CAM_Think` already replicates this verbatim.
2. `V_CalcNormalRefdef` takes `cam_ofs` angles → `AngleVectors` → fwd/right/up, then
   positions the camera by the **preset offset** (section A). Because `cam_ofs` yaw
   trails the mouse (MoveToward /4), the whole rig **orbits** the player as you turn,
   with a slight rotational lag.
3. **CRITICAL for C:** at the end of the third-person block (0x67d60 region) TS
   **overwrites the render viewangles with the smoothed cam_ofs angles**:
   `viewangles[PITCH]=cam_ofs_pitch; [YAW]=cam_ofs_yaw; [ROLL]=0`. So the *rendered
   camera orientation lags the instant mouse aim.* It also derives a pitch value and
   writes it into localplayer fields `+0xb54/+0x2cc/+0x178/+0xb28` (the body/gait
   aim so your model pitches to match) — provenance only, not needed for the view.
4. Wall containment: the stock `cam_contain` trace path is **compiled out** in TS's
   V_CalcNormalRefdef for the normal branch (there's a `PM_TraceLine`-style call only
   in the `back==0` special/viewmodel sub-branch). i.e. TS's default camera does
   **not** wall-trace the offset. Our `cam_contain` block is already `#ifdef LATER`
   (off) — leave it off; that matches. (MED confidence — no evidence of an active
   contain trace on the main path.)

### Stock-vs-TS summary

- **Same as stock:** CAM_Think angle smoothing, `thirdperson` toggle, CL_CameraOffset,
  `cam_snapto`/`cam_ideal*` cvars, no wall-contain.
- **TS changed:** the *position* math in V_CalcNormalRefdef — replaced pure
  `-dist*forward` with the `cl_camtype` preset **back+right+up** offset, added the
  dynamic back-ease, and **sets render viewangles = smoothed cam_ofs angles**.

### Drop-in (B) — replace the 3rd-person block in `cl_dll/view.cpp`

Current (≈ lines 789–807):
```cpp
else if (0 != CL_IsThirdPerson())
{
    Vector ofs; ofs[0]=ofs[1]=ofs[2]=0.0;
    CL_CameraOffset((float*)&ofs);
    VectorCopy(ofs, camAngles); camAngles[ROLL]=0;
    AngleVectors(camAngles, camForward, camRight, camUp);
    for (i=0;i<3;i++) pparams->vieworg[i] += -ofs[2]*camForward[i];
}
```
Faithful replacement:
```cpp
else if (0 != CL_IsThirdPerson())
{
    Vector camOfs; camOfs[0]=camOfs[1]=camOfs[2]=0.0f;
    CL_CameraOffset((float*)&camOfs);         // camOfs[0]=pitch, [1]=yaw (smoothed)

    VectorCopy(camOfs, camAngles); camAngles[ROLL]=0;
    AngleVectors(camAngles, camForward, camRight, camUp);

    // cl_camtype preset (RE: client.dll V_CalcNormalRefdef 0x67fb0)
    float back, side, lift;
    switch ((int)cl_camtype->value) {
    case 1:  back=-48.0f; side=-16.0f; lift=TSCAM_UP_DEFAULT; break;
    case 2:  back=-90.0f; side=  0.0f; lift=TSCAM_UP_DEFAULT; break;
    case 3:  back=-cam_back->value; side=cam_right->value; lift=cam_up->value; break;
    default: back=-48.0f; side= 16.0f; lift=TSCAM_UP_DEFAULT; break; // camtype 0
    }
    if (back < TSCAM_BACK_MIN) back = TSCAM_BACK_MIN;   // [-128,24]
    if (back > TSCAM_BACK_MAX) back = TSCAM_BACK_MAX;
    if (side < -TSCAM_SIDE_CLAMP) side = -TSCAM_SIDE_CLAMP;   // [-24,24]
    if (side >  TSCAM_SIDE_CLAMP) side =  TSCAM_SIDE_CLAMP;
    if (lift < -TSCAM_SIDE_CLAMP) lift = -TSCAM_SIDE_CLAMP;
    if (lift >  TSCAM_SIDE_CLAMP) lift =  TSCAM_SIDE_CLAMP;

    for (i=0;i<3;i++)
        pparams->vieworg[i] += back*camForward[i] + lift*camUp[i] + side*camRight[i];

    // TS makes the RENDER camera use the SMOOTHED cam_ofs angles (this is the lag
    // source for the reticle in C). Without this our camera tracks instantly.
    VectorCopy(camAngles, pparams->viewangles);   // camAngles = (pitch,yaw,0)
}
```
> The dynamic back-ease (48→92) is **omitted** above pending the live read of the
> `0x130c30` gate — start with the static preset, then layer the ease once the gate
> is identified. Mark it `// VERIFY dynamic zoom gate`.
> Also note: setting `pparams->viewangles=camAngles` here changes the whole view to
> lag slightly. If that reads as too floaty on the *camera* (vs the reticle), the
> lag may be reticle-only — resolve with the live trace in C before shipping.

---

## C. RETICLE LAG — the 3rd-person floating crosshair

### Status: NOT statically pinnable (LOW–MED confidence). Live trace required.

The TS reticle draw lives inside a **3.2 KB studio/HUD render function**
(`FUN_10032540`, the one the prior agent flagged "decompiles unusably" — Ghidra
loses the stack frame; it's the player-model + HUD composite). The exact per-frame
crosshair screen (x,y) computation could **not** be recovered from the static
decompile. `WorldToScreen` is reached through the engine func table (not an import),
so it can't be grep'd either.

### Mechanism hypothesis (MED confidence — derived, not read)

We DID establish the necessary precondition (section B.3): in 3rd person TS renders
with **smoothed `cam_ofs` viewangles** while the gun still fires along the **instant**
`cl_viewangles`. Two faithful ways the "float behind mouse" emerges:

1. **Projection-parallax (most likely):** the reticle is the `WorldToScreen()`
   projection of the *aim point* (a trace from the eye/muzzle along the INSTANT
   `cl_viewangles` forward, out to the trace-hit distance). Because (a) the camera is
   offset over-the-shoulder and (b) the render view uses the *lagging* cam_ofs angles,
   the projected point sweeps across the screen and settles — i.e. it trails the
   mouse then re-converges. No explicit 2-D lerp needed; the lag is the camera
   smoothing + offset parallax.
2. **Explicit screen-space smoothing:** a stored crosshair (cx,cy) lerped toward the
   target each frame. Possible but unproven; if TS did this, the coefficient must be
   read live.

Our current `ts_reticle.cpp` pins `cx=W/2, cy=H/2` — no projection, no lag. That is
exactly the observed difference.

### Drop-in (C) — best faithful reconstruction (hypothesis 1), MARK AS PROVISIONAL

Replace the pinned centre (ts_reticle.cpp ~L98):
```cpp
int cx = ScreenWidth / 2;
int cy = ScreenHeight / 2;

// --- 3rd-person floating reticle (PROVISIONAL — VERIFY vs live trace, task C) ---
// In 3rd person the gun fires along the INSTANT view aim, but the render camera uses
// the SMOOTHED cam_ofs angles (RE: view.cpp/V_CalcNormalRefdef). Projecting the aim
// point through the (lagging, offset) view => the reticle trails the mouse. In 1st
// person we keep it centred.
if (CL_IsThirdPerson() && !TS_ScopedIn())
{
    float va[3]; gEngfuncs.GetViewAngles(va);          // instant aim
    Vector fwd; AngleVectors(va, fwd, NULL, NULL);
    Vector eye = v_origin;                              // view origin (view.cpp global)
    Vector aim = eye + fwd * 4096.0f;                  // or use a pmtrace hit dist
    // trace to the first solid so the reticle sits ON the target (adds the parallax):
    pmtrace_t* tr = gEngfuncs.PM_TraceLine(eye, aim, PM_TRACELINE_PHYSENTSONLY, 2, -1);
    if (tr && tr->fraction < 1.0f) aim = tr->endpos;
    float scr[2];
    if (gEngfuncs.pTriAPI->WorldToScreen((float*)&aim, scr) == 0) { // 0 == on-screen
        cx = (int)((1.0f + scr[0]) * 0.5f * ScreenWidth);
        cy = (int)((1.0f - scr[1]) * 0.5f * ScreenHeight);
    }
}
```
`WorldToScreen` uses the *current frame's* (lagging) view matrix, so the trail is
automatic. If the observed lag is LARGER than this parallax produces, add an explicit
screen-space smoothing:
```cpp
static float s_cx=0, s_cy=0; // persist
const float k = 0.25f;       // VERIFY coefficient via live watch
s_cx += (cx - s_cx)*k; s_cy += (cy - s_cy)*k;
cx=(int)s_cx; cy=(int)s_cy;
```

### The EXACT live trace to settle C (for `scripts/live.py` on the running hl.exe)

You (parent) run these; do not guess the coefficient. The original TS client is
`ts/cl_dlls/client.dll` in the live `hl.exe` (Wine). Base via
`binmap.py`/`live.py base client.dll`.

1. **Is it WorldToScreen-based?** Break on the engine `WorldToScreen` used by the HUD
   redraw while in 3rd person, and read the world vec passed in (ECX/stack) + the
   returned screen coords each frame. If the world vec = eye + k*forward(cl_viewangles),
   hypothesis 1 is confirmed and you can read the trace distance used.
   - Find the call: `dc.sh client` around the HUD reticle draw; the engine func
     table slot for WorldToScreen is in `pTriAPI`. Alternatively `live.py break` the
     reticle sprite draw (SPR_Set of `cross2.spr`) and walk back up the stack.
2. **Measure the lag coefficient directly:** `live.py watch` the drawn crosshair
   screen (x,y) — if TS stores it as a client global (a smoothed 2-D coord), watching
   it while flicking the mouse a known delta gives the per-frame convergence ratio
   (that's `k`). If there is NO stored 2-D coord (pure projection), you'll see no such
   global change — that itself confirms hypothesis 1.
3. **Read the aim source:** at the reticle-draw entry, dump `cl.viewangles` vs the
   render `r_refdef.viewangles`; the divergence = the camera-smoothing lag that drives
   the float. Confirms whether the reticle uses instant or smoothed angles for the aim
   ray.

Deliver (world vec formula + trace distance + any k) back and I finalize the C
drop-in with HIGH confidence.

---

## Confidence table

| Claim | Conf | Basis |
|---|---|---|
| `cl_camtype` selects 0–3 offset presets | HIGH | disasm 0x67fb0–0x68033 + cvar strings |
| mode0 = back-48/right+16/up+12 | HIGH | float byte constants |
| back clamp [-128,24], side/up [-24,24] | HIGH | .rdata consts read |
| `up`=12, cam_back"90"/cam_right"0"/cam_up"12" | HIGH | consts + CAM_Init strings |
| dynamic back ease to -92 under a player gate | MED | disasm read; gate bit 0x130c30 unresolved |
| death cam back ease to -180 | MED | disasm; matches our -80 placeholder intent |
| render viewangles set = smoothed cam_ofs in 3P | HIGH | 0x67d60 block |
| no active wall-contain trace on main path | MED | absence in decompile |
| entry = `thirdperson` toggle, seeds cam_ofs[2]=30 | HIGH | CAM_ToThirdPerson 0x4fe00 |
| reticle = WorldToScreen of instant-aim point, lag from smoothed view | MED-LOW | derived from B.3; draw fn untractable |
| exact reticle lerp coefficient | UNKNOWN | needs live trace |
