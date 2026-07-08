# cont94 — client display cvars: original read sites + fix specs

READ-ONLY RE dossier. Follows `reference/dossiers/cont91/cvar_settings_audit.md` fix-2 ("9 client
display cvars registered but never read"). Everything below is recovered from `reference/client.dll`
(PE imagebase 0x10000000; file offset == RVA for the ranges cited). Registration block: one run in
the post-`HUD_Init` input init at **0x10026f00–0x1002721f** (`pfnRegisterVariable` = `call *0x100d5b60`;
the returned `cvar_t*` is stored by the FIRST `mov %eax, <global>` AFTER each call). Value reads are
`mov <global>,%reg` + `flds 0xc(%reg)` (`cvar_t.value` @ +0xc).

## 0. Registration table (verified against the binary — ALL match our `cl_dll/ts_hud.cpp` defaults)

| cvar | orig default | flags | cvar_t* global | our default (ts_hud.cpp) |
|---|---|---|---|---|
| cl_showequipment | "1" | ARCHIVE | 0x100df574 | "1" ✓ |
| cl_gore | "2" | ARCHIVE | 0x100df3a8 | "2" ✓ |
| cl_motionblur | "1" | ARCHIVE | 0x100df450 | "1" ✓ |
| **cl_thirdtransparency** | **"1"** | ARCHIVE | 0x100df534 | "1" ✓ |
| cl_solidcross | "2" | ARCHIVE | 0x100df510 | "2" ✓ |
| cl_laser | "0" | ARCHIVE | 0x100df5b0 | "0" ✓ |
| cl_lasercolor | "10" | ARCHIVE | 0x100df44c | "10" ✓ |
| cl_thirdpersonswitch | "0" | ARCHIVE | 0x100df3a4 | "0" ✓ |
| cl_wavedetail | "0" | ARCHIVE | 0x100df550 | "0" ✓ |
| cl_fx | "1" | ARCHIVE | 0x100df464 | "1" ✓ |

Adjacent in the same run (context): cl_attackswitch "0", cl_thirdperson "0", cl_usepowerup "1",
cl_shadow "2", cl_realhud "0", cl_buykevlar "1", nohud "0" (flags **0**, not ARCHIVE — ours says
ARCHIVE, cosmetic), hud_r "150" / hud_g "175" / hud_b "255", hud_color "150,175,255", cl_camtype "0".
**The `.scr` defaults (user.scr shows cl_thirdtransparency 0, cl_wavedetail 0 …) are only the
listen-server GUI suggestions; the cvar_t registration default is the parity target and ours match.**

Constants used below: 0x1007a910=100.0f · 0x1007cae0=255.0f · 0x10079e58=1.0f · 0x10079e88=10.0f ·
0x1007a8d0=5.0f · 0x1007a870=0.5f · 0x1007d74c=2.5f · 0x1007d750=33.0f · 0x1007d4e8=0.99f ·
0x100799f0=0.0f. `DAT_100963e8` = the current game-speed/slow-mo scale (heartbeat/blur key off it).

---

## 1. ★ cl_thirdtransparency — 3rd-person self-transparency gate (PRIORITY)

**Read site:** 0x678e6, inside **V_CalcNormalRefdef = FUN_10067890** (binmap-confirmed name), i.e.
exactly the region our commit 8d837c2 ported. Disasm of the gate (0x678cd–0x6798a):

```
call 0x1004c250                 ; FirstPersonState() — nonzero => NOT rendering self in 3rd person
test al,al ; jne 0x1006798c     ; -> lp->curstate.rendermode=0 (kRenderNormal), renderamt=0xff
; --- third-person path ---
mov  al,[0x1018a60c]            ; reticle-over-player flag: 0 none / 1 teammate / 2 enemy
test al,al ; je opaque          ;   (written by the ID-trace fn FUN_1004acc0 — our ts_reticle)
mov  eax,[0x100df534]           ; cl_thirdtransparency cvar_t*
flds 0xc(%eax) ; call __ftol    ; (int)value
test eax,eax ; je opaque        ; ZERO => opaque target
  target = 100.0f  (0x1007a910) ; aiming at a player AND cvar on
opaque:
  target = 255.0f  (0x1007cae0)
DAT_10097210 = target
step = (DAT_10097214 < target) ? 5 : 50        ; fade-out (toward 100) 50/frame, fade-back 5/frame
DAT_10097214 = Approach(target, DAT_10097214, step)   ; FUN_10057e90: pure step, no frametime
lp->curstate.rendermode = 1     ; kRenderTransColor — ALWAYS on the 3rd-person path
lp->curstate.renderamt  = (int)DAT_10097214
```

- Test form: **`(int)cl_thirdtransparency->value != 0`** (ftol truncation — 0.5 reads as OFF).
- The cvar only selects the TARGET alpha; rendermode=1 + the fade run regardless in 3rd person.
- The aim flag is `!= 0` — teammate (1) AND enemy (2) both fade (matches our `g_iTSAimTarget != 0`).
- `FUN_1004c250` = spectator checks (`DAT_100d5c7c[0/1]()`) OR camera-object thirdperson member
  (`0x100d43cc+0x20`) == 0 → "first person" → hard reset rendermode 0 / amt 255. NOTE: the original
  does **NOT** reset the smoothed alpha `DAT_10097214` on this path — the fade value persists.

**Fix spec — `cl_dll/view.cpp` (~line 695):**
1. Cache once: `static cvar_t* s_thirdtrans = gEngfuncs.pfnGetCvarPointer("cl_thirdtransparency");`
2. Change `float target = (g_iTSAimTarget != 0) ? 100.0f : 255.0f;` to
   `float target = (g_iTSAimTarget != 0 && (int)s_thirdtrans->value != 0) ? 100.0f : 255.0f;`
3. Registration default stays "1" (already correct — do NOT change to the .scr's 0).
4. (Secondary fidelity, optional) the hard-reset branch should ALSO fire when in FIRST person
   (`0 == CL_IsThirdPerson()`), and should NOT reset `s_alpha` (the original leaves DAT_10097214
   untouched; ours zeroes it to 255). Our step logic (`>=target ? 50 : 5`) already matches; the
   VERIFY on line 697 can be removed — 50-down/5-up is confirmed at 0x67911–0x6794b.

---

## 2. cl_showequipment — passive weapon-bar overlay visibility/alpha

**Read site:** 0x4e047, top of **CHudTSWeaponBar::Draw (0x4e030)** (binmap 1.00). The arg's low
byte is a passive/active flag: the caller (0x4d4a8–0x4d4b3) passes **0** when the weapon-select
menu is open (`this+0x73c` selecting && not spectating), **1** otherwise (passive equipment list).

```
if (passive) {
  switch ((int)cl_showequipment->value) {
    case 1: alphaScale = 0.25f; break;     // 0x3e800000
    case 2: alphaScale = 0.5f;  break;     // 0x3f000000
    case 3: alphaScale = 1.0f;  break;     // 0x3f800000
    default: return;                        // 0 (or anything else) = hidden
  }
} else alphaScale = 1.0f;                   // select menu open: never gated
```
alphaScale multiplies the passive draw alphas (headers 100/50 etc. — the `fildl`+`fmul` pairs
through the body).

**Fix spec — `cl_dll/ts_weaponbar.cpp` `CHudTSWeaponBar::Draw` (~line 328):** in the
`!m_selecting` (passive) path, read `(int)cl_showequipment->value`: not in {1,2,3} → `return true`;
else scale every passive-mode alpha by 0.25/0.5/1.0. The `m_selecting` path is never gated.

## 3. cl_gore — blood-spray particle count (PTakeDam tail)

**Read site:** 0x48abb in **FUN_10048840** = the blood spray (`sprites/bloodfrag2.spr`, plus an
extra `gun_smoke_add` puff for the bit7 upper-body variant). Called ONLY from the **"PTakeDam"**
user-message handler (0x4bcd0, hooked at 0x49c3b): 3 COORD hit point, 3 COORD dir, 1 byte encoded
damage → `FUN_10048840(point, dir, encdmg)`. Count logic (disasm 0x48abb–0x48b93), where `base`
is derived from the damage byte (bit7: `base=(base-128)*2` then +random and halved — see 0x48a90):

```
v = cl_gore->value
if (v <= 10.0f) {
  switch ((int)min(v, 5.0f)) {     // 6..10 clamp to 5
    case 1: count = base * 0.5f;
    case 2: count = base;          // default "2"
    case 3: count = base * 2.5f;
    case 4: count = base * 5.0f;
    case 5: count = base * 10.0f;
    default: count = 0;            // v == 0 → NO blood
  }
} else count = base * min(v, 33.0f);   // hidden super-gore tail
```

**Fix spec:** the consumer is **un-reconstructed** — our `CHudHealth::MsgFunc_PTakeDam`
(`cl_dll/health.cpp:146`) already flags the FUN_10048840 tail as VERIFY and only feeds the pain
indicator. When that blood tail is reconstructed (ts_particles), apply the count table above.
Until then: **register-only is correct**; do not invent a gate elsewhere.

## 4. cl_motionblur (+ its cl_fx coupling) — slow-motion player ghost trails

Three read sites, one subsystem (all un-reconstructed in ours; server DOES send TSMBlur —
`dlls/ts_stunt.cpp` / `ts_player.cpp` / `ts_gun.cpp` — our client never hooks it):
- **0x29680 — FUN_10029680(this=&DAT_100dff48 blur array, idx, level) — the ARM fn**, called only
  by the **"TSMBlur"** user-message handler (0x4bf90, wrapper 0x49b00, hook 0x49cb9):
  READ_SHORT player idx, READ_BYTE level. `if ((int)cl_motionblur->value == 0) return;` else per
  player entry (0x18 stride: {active, count, frames, double time}): already active → frames += 5;
  else `switch ((int)min(level, cl_fx->value))` (**the 0x296b6 cl_fx read**): 1 → frames=10,
  2 → frames=15, else → 5; active=1, count=0.
- **0x29789 — FUN_10029780 — per-frame ghost spawner** (32-entry loop): gate
  `(int)cl_motionblur->value != 0`; per active due entry, ONLY while in slow motion
  (`DAT_100963e8 <= 0.99f`), every ~0.03 s spawns a TEMPENT copy of the player's model/state
  (renderamt 120.0, life ~0x78, fade callback @0x29710) until count reaches frames.
- **0x31c41 — inside StudioDrawPlayer-equivalent FUN_10031ae0**, the ghost-entity draw branch
  (`entstate+0x302 == 1` marks a ghost): skip drawing YOUR OWN ghosts unless
  `CL_IsThirdPerson() && (int)cl_motionblur->value >= 2`; also skipped entirely when not in
  slow-mo (`0.99f < DAT_100963e8`).

Semantics: **0 = off, 1 = others' trails (and yours invisible in 1st person), 2 = also your own
trails in 3rd person** (and 15 arm-frames cap via the min() with cl_fx).

**Fix spec:** feature un-reconstructed client-side. When built (hook "TSMBlur" in a new
`cl_dll` blur module + tempent spawner): apply the three gates exactly as above. Register-only
for now. (Wire format confirmed: short playerindex, byte level.)

## 5. cl_fx — special-effects detail

Read sites: 0x296b6 (blur frames cap — §4), 0x4716c, 0x56dea, 0x56e45, 0x57d5f:
- **0x4716c — FUN_10047130** = the muzzle/gun smoke spawner (`sprites/gun_smoke_add.spr`, 5-puff
  loop), called from ~12 weapon fire event handlers: gate **`cl_fx->value != 0.0f`** (plus PVS
  visibility). cl_fx==0 ⇒ no muzzle smoke.
- **0x56dea/0x56e45 — FUN_10056dd0** = the per-frame FX render root (ends `glDisable(GL_FOG)`):
  first frame or whenever `cl_fx->value != cached (DAT_1018cac4)` → re-run FUN_10057d50; and
  `if (cl_fx->value == 1.0f) FUN_10057a70();` — an extra per-frame pass ONLY at exactly 1
  (VERIFY its identity when the FX root is reconstructed).
- **0x57d5f — FUN_10057d50** = particle-system global config: `value ∈ {1.0, 2.0}` → hi config
  (max 0x1000 particles, white 1.0/0.05/0.05/0.05, sizes 1.5/30.0, flags 1/1, count 1); else
  (0 or ≥3) → low config (max 0x800, 0.35/0.2/0.3/0.2, 0.7/5.0, flags 0/0, count 8); caches the
  value in DAT_1018cac4.

**Fix spec:** immediate actionable piece = the **muzzle smoke gate**: our fire-event impact/muzzle
smoke (`cl_dll/ev_ts.cpp:485` region and `ts_particles.cpp:520/528/535` TSP_Smoke calls that
correspond to FUN_10047130 call sites) must be wrapped in
`if ((int)gEngfuncs.pfnGetCvarFloat("cl_fx") != 0)` (exact original test: float != 0.0). The
FX-root reconfigure + the ==1 extra pass belong to the un-reconstructed TSP config layer —
document in ts_particles when that lands. The blur coupling rides §4.

## 6. cl_solidcross — crosshair style

**Read site:** 0x4ed01 (via esi) in **FUN_1004e9f0** = the crosshair draw (this=CHudTSWeaponBar
0x100d0ca0; called at 0x4d517 right after the laser-dot fn). Sprite handles, paired by the
store-after-call pattern in the init at 0x4cbd1–0x4cbfe: **+0x14c = sprites/cross_e.spr**,
**+0x150 = sprites/cross2.spr**, **+0x154 = sprites/cross_t.spr**.
Draw logic (weapon != 0/0x18/0x19/0x22/0x23):
- over TEAMMATE (flag 0x1018a60c==1): SPR_Set(+0x154 cross_t, colors) + draw — no cvar.
- else: 4 gap bars FillRGBA(…, 5x2 / 2x5, r,g,b, alpha **0x4b=75**) at ±spread offset; then
  `if (IsHardwareGL() && cl_solidcross->value > 0.0f)`: SPR_Set(**+0x150 cross2.spr**, r,g,b) with
  **all three components forced 0 (BLACK) when value > 1.0f**, `SPR_DrawHoles`-family at
  (cx-0x20, cy-0x20); `else`: SPR_Set(**+0x14c cross_e.spr**, (int)hud colors) + the additive draw
  at the same spot.
- Colors r/g/b: enemy-flag==2 → red 0xc3,0x2d,0x2d; teammate → green; else hud_r/g/b ftol'd.

Semantics: **0 = additive sprite crosshair (cross_e), 1 = solid cross2 in HUD color, 2 (default)
= solid cross2 BLACK.** Software mode always uses the else path.

**Fix spec — `cl_dll/ts_reticle.cpp` (~line 174):** replace the unconditional near-black
`SPR_Set(m_hCross2, 16,16,16)` with: `v = (int)cl_solidcross->value` → v==0: draw cross_e.spr
additive tinted hud color; v==1: cross2 SPR_DrawHoles tinted hud color; v>=2: cross2 with (0,0,0)
(original uses literal black, not (16,16,16)). Load cross_e.spr alongside m_hCross2.

## 7. cl_laser + cl_lasercolor — BOTH real, different jobs

**cl_laser ("Lasersight style", default 0)** — 5 read sites, 2 functions:
- **0x3bc9b / 0x3bdde in FUN_1003bb00** (the world laser/attachment renderer, 5758 B — our
  ts_attachments): beam direction = `if (viewmodel==0 || cl_laser->value == 1.0f)` (bit-exact
  compare vs 0x3f800000) → the VIEW forward (`DAT_1018bd90..98`); `else` → normalized
  (attachment2−attachment1) of the gun model + per-frame random jitter (rand(-1,1) · const).
  Two sites = the two akimbo/beam segments.
- **0x4e7f6 / 0x4e8a9 / 0x4e950 in FUN_1004e6f0** (the local laser HUD dot(s), called at 0x4d3f4;
  `this+0x14 & 2` = laser active, `& 0x40` = akimbo → two dots; `+0x1c/+0x20` and `+0x28/+0x2c`
  = projected screen pos, sentinel -4.0f): each dot:
  `if (settledFlag DAT_1018a628 != 0 && cl_laser->value < 1.0f)` → FillRGBA 2x2 at the CROSSHAIR
  center + per-frame ±1px jitter (rand(-10,10)/10); else → FillRGBA 2x2 at the (int)projected
  beam-end. DAT_1018a628 comes from the predicted-weapon txfer (0x3b9c region; zeroed while
  `wd+0x1d0 <= 0`) — "laser settled on aim" // VERIFY exact field.
  Dot color: `FUN_100582f0(DAT_1018d128[localidx*2+1])` — the **server-echoed** per-player color
  byte, NOT cl_lasercolor directly.

**cl_lasercolor (default "10")** — ONE read site, 0x4ce10 = first instruction of FUN_1004ce10
(the CHud think: usepowerup double-tap, slow-mo heartbeat/breathe, weapon-switch sound):
change-detect vs `this+0x3728`, on change → `ClientCmd("lasclr_%i", v)` (string 0x95e08 confirms
the underscore form our `ts_attachments.cpp:383` already sends). All laser RENDER color comes back
via the per-player byte (PlInfo), **including the local player's**.

**Fix spec — `cl_dll/ts_attachments.cpp`:**
- The `lasclr_%i` announcer (line 373-383) is correct — keep.
- Replace the direct `cl_lasercolor` reads at lines 416 and 513 with the server-echoed
  `g_playerLaserColor[lp->index]` for the LOCAL player too (drop the `isLocal ?` special case at
  416; line 513 likewise) — the original never renders from the cvar, so a server-side override
  or pre-echo state renders faithfully (color index 0 until the echo arrives).
- Style gate: our beam/`TS_LocalLaserDot` currently always uses the traced gun endpoint =
  original's `cl_laser==1` path. Add the default (0) behaviours: beam dir from the viewmodel
  attachments + jitter when `cl_laser->value != 1.0f` (our LocalLaserEndpoint may already do
  this — align the condition), and the HUD dot snapping to the reticle center ± 1px jitter when
  the settled flag holds (our nearest equivalent: the beam endpoint trace reaching the aim point;
  mark VERIFY, needs a live A/B trace to pin DAT_1018a628 exactly).

## 8. cl_thirdpersonswitch — auto 3rd-person on bare hands

**Read sites:** 0x4f93b / 0x4f958 in **FUN_1004f800** = the **"WeaponInfo"** user-message handler
(hooked at 0x4c9d1 on CHudTSWeaponBar 0x100d0ca0). After reading the new weapon id into
`this+0x7b8` (old id saved first):

```
if (newId < 1) {                        // 0 = bare hands / kung fu
  if (cl_thirdpersonswitch->value == 1.0f    // BIT-exact vs 0x3f800000
      && !CL_IsThirdPerson())
    ToggleThirdPerson();                // FUN_1004fe00
} else {
  ... slot bookkeeping ...
  if (cl_thirdpersonswitch->value == 1.0f && CL_IsThirdPerson()
      && oldId == 0)                    // leaving bare hands
    ToggleThirdPerson();
}
```
FUN_1004fe00 = camera toggle (flips DAT_100df390, saves angles, dist 30.0, "cam_command"; skips
while spectating) — ours: `CAM_ToThirdPerson()` / `CAM_ToFirstPerson()` (`cl_dll/in_camera.cpp`).

**Fix spec — `cl_dll/ts_ammo.cpp` `CHudTSAmmo::MsgFunc_WeaponInfo` (line 149):** capture
`const int oldType = m_iWeaponType;` before the READ; after updating:
```
if (gEngfuncs.pfnGetCvarFloat("cl_thirdpersonswitch") == 1.0f) {
    if (m_iWeaponType == 0 && 0 == CL_IsThirdPerson()) CAM_ToThirdPerson();
    else if (m_iWeaponType > 0 && 0 != CL_IsThirdPerson() && oldType == 0) CAM_ToFirstPerson();
}
```
(Exact ==1.0f test; value 2 must NOT trigger.) Note the original toggle is skipped in spectator —
CAM_* already no-op appropriately; verify.

## 9. cl_wavedetail — flying-bullet trails (realbullet client sim)

**Read site:** 0x35a66 in **FUN_10035010** (2787 B, called from ~0x35bf1) = the CLIENT bullet
manager update (per-bullet 0x5c-stride entries; simulates the tracked "realbullet" projectiles).
Gate: `if (cl_wavedetail->value > 0.0f && time > entry->nextFxTime)` → spawn a trail/wave effect
(FUN_10046240) along the bullet's path, rethrottled +0.01 s (FUN_10056fa0(0.01)).

**Fix spec:** **feature un-reconstructed** — our client has no bullet manager (the cont91 audit's
write-only `g_iTSRealBullet` is the same gap). When the client realbullet sim is reconstructed,
gate its trail FX on `cl_wavedetail->value > 0`. Register-only until then. (Note the .scr calls it
"Bullet trails?"; it does NOT gate tracers or the server sim.)

---

## Summary — actionable now vs deferred

| cvar | consumer in ours | action |
|---|---|---|
| cl_thirdtransparency | `view.cpp:695` | **ADD the `(int)value != 0` gate** (spec §1) — 3-line fix |
| cl_showequipment | `ts_weaponbar.cpp` Draw passive path | gate+alphaScale (spec §2) |
| cl_solidcross | `ts_reticle.cpp:174` | 3-style branch + cross_e.spr (spec §6) |
| cl_thirdpersonswitch | `ts_ammo.cpp:149` | toggle logic (spec §8) |
| cl_fx (muzzle smoke part) | `ev_ts.cpp` / `ts_particles.cpp` smoke calls | `!= 0` gate (spec §5) |
| cl_laser / cl_lasercolor | `ts_attachments.cpp` | color source swap + style gate (spec §7, part VERIFY) |
| cl_gore | PTakeDam blood tail — un-reconstructed | defer; multiplier table recorded (§3) |
| cl_motionblur | TSMBlur client subsystem — un-reconstructed | defer; full mechanism recorded (§4) |
| cl_wavedetail | client realbullet sim — un-reconstructed | defer (§9) |

All 10 registration defaults already match the binary — no ts_hud.cpp changes needed (optional
cosmetic: `nohud` is registered with flags 0, not FCVAR_ARCHIVE, in the original).
