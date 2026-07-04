# cont.86 — Gait playback speed + the "ombra" blob shadow (client.dll RE)

Two independent client-side presentation gaps. Both RE'd from `reference/client.dll`
(imagebase 0x10000000). Raw decompiles in `decomp/`:
`gait_estimate_process.c`, `shadow_ombra.c`, `hud_drawtransparent.c`.

---

## QUESTION 1 — walk/run gait plays too fast (not velocity-scaled)

### Where it lives in the original
`client.dll` reimplements the whole studio renderer (`CStudioModelRenderer`, object
`g_StudioRenderer` @0x100db0e0). The two gait functions:

| Fn | RVA | Role |
|---|---|---|
| `StudioEstimateGait(pplayer)` | **0x1FA0** (`0x10031fa0`) | computes `m_flGaitMovement` + `gaityaw` |
| `StudioProcessGait(pplayer)`  | **0x2250** (`0x10032250`) | controllers + **gait-frame advance** |

Renderer member offsets recovered from these fns:
`+0x08` m_clTime (double) · `+0x10` m_clOldTime (double) · `+0x1c` **m_fGaitEstimation** (int)
· `+0x20` m_nFrameCount · `+0x30` m_pCurrentEntity · `+0x38` m_pPlayerInfo · `+0x40` m_flGaitMovement.
player_info: `+0x180` gaitframe · `+0x184` gaityaw · `+0x188` prevgaitorigin.

### The gait-frame advance (the core of the question)
From `StudioProcessGait` @0x2250, the tail (verified against raw disasm 0x100324a8-0x100324ce):

```c
pseqdesc = &seqdesc[ pplayer->gaitsequence ];         // (curstate.gaitsequence @ +0xb8)
if (pseqdesc->linearmovement[0] <= 0.0)               // seqdesc +0x4c
    gaitframe += m_pCurrentEntity->curstate.framerate  // entity +0x310  <-- SEE BELOW
                 * pseqdesc->fps                        // seqdesc +0x20
                 * dt;                                  // m_clTime-m_clOldTime, clamped [0,1]
else
    gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes; // +0x38
```

`entity+0x310` is **`curstate.framerate`** (curstate begins at cl_entity+0x2b0 — proven by
`curstate.controller[0]` @ +0x318 = entity_state offset 0x68 ⇒ curstate@0x2b0; framerate is
entity_state offset 0x60 ⇒ 0x2b0+0x60 = **0x310**). Read with `flds` (a float). **Confidence: HIGH.**

### OUR code (`cl_dll/StudioModelRenderer.cpp:1435-1442`)
```c
if (pseqdesc->linearmovement[0] > 0)
    m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
else
    m_pPlayerInfo->gaitframe += pseqdesc->fps * dt;      // <-- MISSING the framerate multiplier
```

### DIVERGENCE 1 (primary, HIGH confidence) — the in-place (linearmovement==0) branch
- **Distance branch (linearmovement>0) is byte-identical** to the original → for gaits that carry
  linear movement, our playback already scales to velocity correctly.
- **The `else` branch differs:** original = `curstate.framerate * fps * dt`; ours = `fps * dt`.
  We **drop `curstate.framerate`**. An in-place walk/run gait (linearmovement==0) is authored to be
  driven at a rate the *server* sets via the player entity's `framerate`. Because we ignore it, the
  in-place gait always plays at the sequence's full authored `fps` regardless of move speed ⇒
  **"walk2 plays too fast / not velocity-scaled."** This is the exact symptom.

### DIVERGENCE 2 (secondary, MEDIUM confidence) — m_fGaitEstimation default
`StudioEstimateGait` @0x1fa0 has both SDK paths, gated on `this+0x1c` (m_fGaitEstimation):
```c
if (m_fGaitEstimation == 0) {                 // <-- the path the original's code lays out first
    m_flGaitMovement = Length(pplayer->velocity) * dt;   // velocity @ pplayer +0x70/+0x74
    ... atan2 gaityaw ...
} else {                                       // origin-delta estimation + (move/dt < 5) clamp
    dxy = origin - prevgaitorigin; prevgaitorigin = origin;
    m_flGaitMovement = Length(dxy);
    if (!(dt>0 && m_flGaitMovement/dt >= 5.0)) m_flGaitMovement = 0;
}
```
OUR code sets **`m_fGaitEstimation = true`** (`StudioModelRenderer.cpp:89`) ⇒ we take the
origin-delta estimation path. The original's Init that writes `+0x1c` was not pinned in this pass
(the CStudioModelRenderer ctor runs at CRT static-init and wasn't isolated; several `mov [reg+0x1c],imm`
sites exist but none provably the renderer). Both paths yield ~distance-per-frame, so this is unlikely
to be the 2× speed cause on its own — but for byte-fidelity the flag's value should be matched.
**Recommend a live trace** (`live.py`, read `g_StudioRenderer+0x1c`) to settle it; treat as secondary.

### Fix recipe (Q1)
`cl_dll/StudioModelRenderer.cpp`, `StudioProcessGait`, the gait-frame else branch (~line 1441):
```c
else
    m_pPlayerInfo->gaitframe += m_pCurrentEntity->curstate.framerate * pseqdesc->fps * dt;
```
That is the exact original formula and the direct "scale to move speed" knob (the server drives
`curstate.framerate` for the walking player). Ship this first and re-test walk2.
Optionally (byte-fidelity, after a live check confirms the original value) flip
`m_fGaitEstimation` at `StudioModelRenderer.cpp:89` to match — do NOT flip blind.

> Caveat: which branch walk2 actually uses depends on whether the TS player model's `walk2`
> sequence has `linearmovement[0] == 0`. Given the symptom (fixed too-fast rate), it almost
> certainly does. If a model dump shows `linearmovement>0`, then Q1 is entirely DIVERGENCE 2
> and the framerate fix is a no-op for that sequence — verify the sequence's linearmovement.

---

## QUESTION 1 — SERVER-SIDE follow-up (walk2/run2 too fast) — DEFINITIVE RE

The coordinator reframed Q1 as server-side (static walk maxspeed + reduced `pev->framerate`). I RE'd
`ts_i386.so` end-to-end. **The reframed hypothesis is NOT supported by the binary — there is no
server-side static walk maxspeed and no server-side walk framerate reduction.** Raw decompiles in
`decomp/` (`GetSpeedBySlots.c`, `SetPlayerSpeed.c`, `Animate_walkrun.c`, `PreThink_framerate.c`,
`GiveSlowMul.c`, `PM_CheckParamters.c`, `TSHandle.c`). What the server actually does:

### 1. `pev->framerate` — SLOW-MOTION ONLY, never walk (HIGH confidence)
`CBasePlayer::PreThink` (nm 0x808cc) sets framerate **every frame** from one source:
```c
InterpolateSlowMotion(gpGlobals->frametime, &pev->fuser1, m_flSlowMotionGo); // ease fuser1
pev->framerate = this->vtbl->GiveSlowMul(this);       // @0x8099f  (fstps pev+0x138)
```
`GiveSlowMul` (nm 0x82960) = `return FixedSlowAmt(pev->fuser1);` — a pure function of the slow-motion
factor `fuser1`. Neutral (no slowmo) = 1.0. I searched **every** write to `pev->framerate`
(entvars +0x138) in the whole `.so`: the only player one is this slowmo write; the rest are
`ResetSequenceInfo`(=1.0), buttons, monsters, powerups. **No walk-conditional framerate write exists.**
Our `dlls/player.cpp` PreThink already replicates this exactly (framerate = clamped `fuser1`).
⇒ The "torso is slower in walk2" is **not** a framerate effect. It is the gait: per Q2/dossier-1, TS
extends the gait bone-split to the spine (`"Bip01 Dorso"`), so the **velocity-scaled** gait drives the
whole upper body, not just the legs. Lower walk velocity ⇒ slower gait ⇒ slower torso, no framerate change.

### 2. Move speed — `GetSpeedBySlots` is the RUN cap; SetPlayerSpeed fires on INVENTORY events only (HIGH)
`GetSpeedBySlots` (nm 0x66bd4):
```c
if (p->field_0x7b9 == 0 && p->slots(0x548) < 81)      // 0x51
    return round(0.012345679f * slots * 120.0f + 210.0f);   // 1/81 * slots * 120 + 210
// slots 0..81  ->  speed 210..330  (empty loadout = 81 slots = 330)
```
`SetPlayerSpeed` (nm 0x83068): `clamp(speed,0,330)`; `pfnSetClientMaxspeed(edict, round(speed))`;
stores `m_flSetMaxSpeed`. **Callers (all inventory events, none per-frame, none walk-specific):**
`TSInit`(=330), `PickUpWeapon`/`BuyWeapon`/`DropWeapon`/`ThrowWeapon`/`TakeFlag`/`TakeArmor`/`DropAmmo`
(=`GetSpeedBySlots`). The original PreThink does **not** write `pev->maxspeed` per frame (our
`player.cpp:2094` does, but it is functionally identical since `GetSpeedBySlots` is deterministic —
harmless, not the bug).

### 3. walk2 vs run2 = a pure VELOCITY THRESHOLD of 180 (HIGH)
`CTSAnimation::Animate` (nm 0x5b470) — the gaitsequence dispatch:
```c
speed = |velocity.xy|;                      // sqrt(vx^2+vy^2)
if (!(flags & FL_DUCKING)) {
    if (speed <= 180.0f)  seq = (speed <= 0.0f) ? <idle> : "walk2";
    else                  seq = "run2";
} else                    seq = (speed==0)? "crouch_idle" : "crawl";
pev->gaitsequence = LookupSequence(seq);
```
So the ONLY thing that makes it "walk2" is horizontal speed ≤ 180 (and >0). It sets `gaitsequence`
only — no framerate, no speed change. (Confirms the coordinator's linearmovement note: run2 covers
185/cycle, walk2 69.7/cycle; both distance-scaled client-side per dossier 1.)

### 4. The movement pipeline has NO walk speed clamp (HIGH)
- `PM_CheckParamters` (0xf9c94) is **stock**: clamps forward/side/up to `min(maxspeed, clientmaxspeed)`,
  zeroes them for frozen/stun buttons, and applies the **duck** reduction `×0.5 fwd / ×0.25 side` on
  `FL_DUCKING`. No walk case.
- `PM_PlayerMove` (0xf08d8): `frametime = 0.001*msec; if (fuser1 ≥ 0.1) frametime *= fuser1`
  (slow-motion time-dilation, movement magnitude preserved), then dispatch. No walk case.
- `TSHandle` (0xe96a0, the TS-custom pm hook) scales speed **only** by the TS movement-state bits in
  `pmove->iuser4` (what reads as `pmove+0x208`): bit `TSPM_STOP` → `×0.5` all (slide-damp),
  `TSPM_SLIDE` → forward=0/side×0.3 (prone strafe), plus dive/walljump/duck dispatch. **None is a
  walk key.** Our `pm_shared/TS_pm_shared.cpp::TSHandle` already implements these exactly.

### Conclusion + what actually causes "too fast"
The server/pm code contains **no static walk speed and no walk framerate reduction** — the coordinator's
server hypothesis does not hold. The walk is the **engine input**: holding +speed scales `cmd.forwardmove`
by `cl_movespeedkey` (× `cl_forwardspeed`) to a fixed value that sits below the slot maxspeed, so the
resulting velocity is (a) weapon-independent ("static") and (b) ≤180 ⇒ selects walk2. The gait then
plays velocity-scaled (distance branch, dossier 1), which also drives the torso via the extended
bone-split. There is nothing to add in `dlls/`.

**So "walk2 too fast" ⇒ our WALK VELOCITY is higher than the original's for the same walk2**, and the
fix is one of (client-side, not server DLL):
1. **The client gait `m_fGaitEstimation` (dossier-1 DIVERGENCE 2).** We default it `true` (origin-delta
   estimation); the original's EstimateGait velocity path (flag==0) uses `Length(pev->velocity)*dt`.
   If our origin-delta estimate over-reads the per-frame movement, the distance-branch gait advances
   too fast for the same real velocity — a *client* fix. **Verify with a live trace** (read
   `g_StudioRenderer+0x1c` in the original; compare `m_flGaitMovement`).
2. **The walk input velocity itself** — compare live `|pev->velocity|` while walking in our build vs the
   original; if ours is higher, the divergence is `cl_forwardspeed`/`cl_movespeedkey`/+speed handling in
   our client/engine (config, not DLL). (With stock `400 * 0.52 = 208 > 180` you'd get run2, so TS must
   ship a lower `cl_movespeedkey`/`cl_forwardspeed` to land walk velocity ≤180 — check the shipped config.)

Recommend: drop the earlier "in-place framerate" client fix (irrelevant — walk2 uses the distance
branch), and diagnose via a live velocity+gaitmovement trace before changing code. No `dlls/` change.

---

## QUESTION 3 — melee-idle 3rd-person head/torso turned LEFT when moving

RE of the server dispatch (`CTSAnimation::Animate`/`SetAnim`), the client controller math
(`StudioProcessGait`), the gait bone-merge (`StudioMergeBones` @0x31080), AND the actual TS
`player.mdl` sequence/bone data. Raw artifacts in `decomp/` (`SetAnim.c`, `Animate_full.c`,
`StudioMergeBones_gait.c`, `melee_idle_model_analysis.txt`).

### What is DEFINITIVELY RULED OUT
- **(b) SetAnim flag — NOT the cause.** `CTSAnimation::SetAnim` (nm 0x5d7e4) decoded: flags
  `LEGS=0x01 CLEARLEGS=0x04 INTERP=0x10 SETFRAME=0x20 FORCE=0x80 KEEPFRAME=0x100` (our map is right;
  `0x12` = INTERP + an unused 0x02 bit). It only sets `pev->sequence`, `frame`, `ResetSequenceInfo` —
  **never touches controllers/blending/body**. The original `Animate` (nm 0x5b470) dispatches BOTH
  the melee override AND the aim_/idle_ torso with the **same flag 0x12** (`Animate_full.c` case 1/6:
  `if (override=="") SetAnim("aim_"/"idle_"+grip, 0x12) else SetAnim(override, 0x12)`) — identical to
  ours. No body-yaw/controller-reset exists on either path.
- **(c) Controller (flYaw) math — NOT the cause.** The original ProcessGait torso controllers =
  `((flYaw * 0.25) + 30.0) * 4.25` — the three constants read from client.dll .rdata are **0.25, 30.0,
  4.25** (= `((flYaw/4)+30)/(60/255)`), **byte-identical to ours** (`StudioProcessGait`). The client
  cannot distinguish the torso sequence anyway — the controller math is sequence-agnostic.
- **Missing-sequence fallback — NOT the cause.** `player.mdl` HAS `knife_idle`/`katana_idle`/
  `briefcase_idle`; and they are **structurally identical** to `idle_onehanded`/`aim_onehanded`
  (all `numblends=2`, `blendtype=(8,0)` = an XR/pitch blend). Same bone controllers (0-3 on
  `Bip01 Dorso`/`Dorso1-3`, type 8 = XR, range ±30°). So it is not a blend-type or structure difference.

### What is CONFIRMED — (a): the melee idles author the head/upper-body looking down-aside
Decoded frame-0 rotations of the head bone `Bip01 Testa` (deg XR=pitch, YR, ZR):
| seq | Testa (XR,YR,ZR) |
|---|---|
| **knife_idle** | **(43.2, -17.0, -18.2)** |
| **katana_idle** | **(62.6, -28.3, -39.9)** |
| idle_onehanded | (21.7, -5.9, -6.9) |
| aim_onehanded | (4.3, -4.2, 7.3) |
The melee idles pose the head sharply DOWN + turned ("looking at the blade") — a standing-idle pose.
The gun idles keep the head near-forward. This is the **visible turned-head pose**, and it is baked
into the animation (same file we and the original both load).

### The one CONCRETE client divergence found — the gait bone-merge
`player.mdl` uses the ITALIAN skeleton: pelvis = `Bip01 Bacino`, spine = `Bip01 Dorso` (bone **8**),
whose children are the ENTIRE upper body (`Dorso1-3 → Collo → Testa`(head) + both arms). There is
**no** `Bip01 Spine` or `Bip01 Pelvis` bone.

Original `StudioMergeBones` (@0x31080, `StudioMergeBones_gait.c`): iterate bones in order, **copy the
gait pose** into each bone until the first `"Bip01 Dorso"`/`"Bip01 Spine"`, then **SLERP-blend that
one split bone** between gait and torso and **`break`** (bones after = torso). For this model that is:
bones 0-7 (root/pelvis/legs) = gait; bone 8 (Dorso) = slerp; bones 9-33 (spine/head/arms) = torso.

**OUR** `StudioModelRenderer.cpp` (~978-1010): a persistent `copy` flag, a **hard copy** (no slerp) that
turns OFF at Dorso, an `else if (parent=="Bip01 Pelvis") copy=true` reset that is **dead code** (the TS
pelvis is `Bip01 Bacino`), and **no break**. Net split boundary is the SAME (bones 0-7 gait, 8+ torso),
but **we omit the spine SLERP at bone 8**. Confidence the slerp exists: HIGH. Confidence it is the FULL
cause: LOW — bone 8's authored gait-vs-torso delta is only ~2-3°, so the slerp alone cannot rotate the
head ~17°; the dramatic turn is the authored head pose above.

### Honest assessment of the cause + how to close it
The turned head is the **authored `knife_idle`/`katana_idle` pose (a)**, which BOTH builds load. Nothing
in the server dispatch (flag, path), the client controllers (identical constants), or the sequence
structure differs — I verified each. So the reason the ORIGINAL renders it "dead-on when moving" is
NOT explained by any static divergence except the small spine slerp. That means the original must
suppress/re-base the melee torso during movement through a path that a static read can't pin (a candidate:
the spine slerp re-basing the upper body toward the gait; or the override being replaced when moving).
Per the "don't invent vs the RE" rule, the decisive step is a **LIVE trace** (the user's running
original): move holding a knife and dump, per frame, `pev->sequence`, `pev->gaitsequence`,
`pev->controller[0..3]`, `pev->blending[0..1]` (server `mp.dll`), plus client `StudioMergeBones`'s
per-bone copy/slerp decision — compare to ours. That will show whether the original (i) plays a
different `pev->sequence` while moving, (ii) sets different controllers/blending, or (iii) the spine
slerp is doing the work.

### Fix recipe (in priority order)
1. **Implement the original's spine SLERP + break** in `cl_dll/StudioModelRenderer.cpp` gait-merge
   (replace the hard `copy`/no-break loop): copy the gait per-bone until the first
   `"Bip01 Dorso"`/`"Bip01 Spine"`, SLERP that bone between gait and torso, then stop. Also drop the
   dead `"Bip01 Pelvis"` reset (harmless but non-faithful). This is the only faithful client change and
   the first thing to try. (MEDIUM confidence it visibly helps; it is correct either way.)
2. If (1) doesn't fully fix it, **live-trace the original moving-with-knife** (above) to capture whether
   the melee override torso is replaced/re-based during movement, then mirror that in
   `dlls/ts_animation.cpp` (`CTSAnimation::Animate`) — do NOT guess it.
- NOT the fix: changing the SetAnim flag (verified 0x12 both) or the controller math (verified identical).

---

## QUESTION 2 — the "ombra" blob shadow sprite under the player (we don't draw it)

### Confirmed: it's `sprites/ombra.spr` (ombra = Italian "shadow")
- String `"sprites/ombra.spr"` @0x10095f84, referenced **once**, at 0x10055801 (the shadow drawer).
- Asset present in content: `recovered/ts3-linux/ts/sprites/ombra.spr` and
  `recovered/tshd3-steam/app/ts/sprites/ombra.spr` (4926 bytes, dated 2007).
- This is **separate** from the engine projected shadow (`GL_StudioDrawShadow`, which we already call).

### The draw path (all in `decomp/shadow_ombra.c`)
| Fn | RVA | Role |
|---|---|---|
| shadow master ("DrawShadows") | **0x5760** (`0x10055760`) | per-frame; loads ombra.spr; loops entities |
| per-entity blob | **0x5500** (`0x10055500`) | traces to ground + draws one quad |
| predicate | **0x579a0** (`0x100579a0`) | `idx in [0,0x20] && shadowFlags[idx]` |

**Call site:** `HUD_DrawTransparentTriangles` (0x34e50, `decomp/hud_drawtransparent.c`) calls the
master `FUN_10055760()` **gated by `IEngineStudio.IsHardware()`** (shadows only in GL/D3D).

**cvar `cl_shadow`** (`decomp/`): registered @0x1002710c, name @0x1009414c, **default value = "2"**
(bytes `32 00` @0x10094158), cvar_t ptr cached at 0x100df4cc. Master reads `cl_shadow->value` (@+0xc):
- **`0`** → early-out, no ombra shadows at all.
- **non-zero** → load ombra.spr, `SpriteTexture`, then loop all entities and draw a blob for each
  (predicate `FUN_100579a0` = a per-entity "wants shadow" flag table `DAT_1018bd44[0..0x20]`).
- **`== 1.0` exactly** → additionally draws the **local player's own** shadow when NOT third-person
  (`!CL_IsThirdPerson() && cl_shadow==1`) — a special-case block before the loop.
- So default **2** = blob shadow for every flagged player, no extra local-first-person one.

### The per-entity blob (FUN_10055500) — geometry/appearance
```
ent = GetEntityByIndex(i);                 // engine cb @DAT_100d5bfc
if (!ent || ent->curstate.messagenum==0) return;
groundZ_probe = ent->origin;  groundZ_probe.z -= 8192.0;          // trace straight down
IEngineStudio.ScreenToWorld/Trace (slots +0x38 setup, +0x3c trace);// hit = ground point
dist = |trace_hit - ent->origin| - 12.0;                          // 12u dead-zone
if (dist < 200.0) {                                               // max shadow distance
    alpha = 1.0 - dist * 0.005;                                   // fades w/ height above ground
    tri.Color4f(1,1,1, alpha);                                    // white * alpha (sprite is the dark blob)
    FUN_10050080(quad, groundPos, ..., size = 24.0, ...);        // build a 24-unit ground quad
    FUN_10050000(quad);                                          // draw it (Vertex3fv/End helper)
}
```
Constants (from .rodata, `objdump -s`): trace-down **8192.0** (0x1007d448) · dead-zone **12.0**
(0x1007d824) · max dist **200.0** (0x1007a938) · base alpha **1.0** (0x10079e58) · alpha falloff
**0.005/unit** (0x1007d820) · quad half-size **24.0** (immediate). Master sets `RenderMode`, `CullFace`,
binds the ombra sprite frame; the per-entity fn only sets color+alpha and emits the quad.
**Confidence: HIGH** on sprite/cvar/size/alpha/placement; the exact triangle-API slot names are inferred
from the standard `triangleapi_s` layout (RenderMode@4, Color4f@0x10, CullFace@0x28, SpriteTexture@0x2c).

### What WE are missing
- **The sprite asset** ships already (ombra.spr in content) — nothing to add there.
- We **register** `cl_shadow "2"` (`ts_hud.cpp:57`) but **never read it and never draw the blob**.
- Our `HUD_DrawTransparentTriangles` (`cl_dll/tri.cpp:52`) draws particles + gun attachments only —
  no shadow pass.
- We only have the engine `GL_StudioDrawShadow` (`StudioModelRenderer.cpp:1795`), which is a
  different effect (projected studio shadow, not the flat ground blob).

### Fix recipe (Q2)
Add a blob-shadow pass, faithful to `FUN_10055760`/`FUN_10055500`, in `cl_dll/tri.cpp`
(`HUD_DrawTransparentTriangles`), only when `IEngineStudio.IsHardware()` and `cl_shadow->value != 0`:
1. On first draw (or in HUD init), `SPR_Load("sprites/ombra.spr")`; keep the model handle.
2. `gEngfuncs.pTriAPI->SpriteTexture(spr_model, 0)`, `RenderMode(kRenderTransAdd/Alpha)`, `CullFace(TRI_NONE)`.
3. For each player/flagged entity: trace from `ent->origin` down 8192u to the ground; compute
   `dist = |hit - origin| - 12.0`; skip if `dist >= 200`; `alpha = 1.0 - dist*0.005`;
   `Color4f(1,1,1,alpha)`; draw a **24-unit** ground-aligned quad centered on the trace hit.
4. Gate: `cl_shadow==0` off; non-zero = all flagged players; `==1` additionally the local player in
   first person. (Match default "2" so it works out of the box.)

Use the existing `TS_Attachments_Draw` triangle-drawing style in `tri.cpp` as the template for the
quad emit. This is a pure client-side presentation add; no wire/protocol change.

---

### Confidence summary
- Q1 DIVERGENCE 1 (missing `curstate.framerate` in the in-place gait branch): **HIGH** — exact formula
  match, offsets proven.
- Q1 DIVERGENCE 2 (m_fGaitEstimation default): **MEDIUM** — original's runtime value not pinned; verify live.
- Q1 which branch walk2 uses: depends on model `linearmovement` — **verify**.
- Q2 sprite/cvar/geometry/placement: **HIGH**. Triangle-API slot names: inferred (standard layout).
