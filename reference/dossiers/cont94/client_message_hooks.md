# cont94 — the 7 missing client user-message hooks (+ PTakeDam blood tail, specmode/special)

READ-ONLY RE dossier (parity_sweep.md §A1). Everything recovered from `reference/client.dll`
(PE imagebase 0x10000000; file offset == RVA), decompiles via `dc.sh client`, disasm via objdump.
Server cross-checks against `reference/ts_i386.so` + our `dlls/` senders. Builds on
`client_display_cvars.md` (do not re-derive its §4 blur mechanics; **one correction to it below**).

## 0. Shared infrastructure (read this first)

**Two hook sites.** All TS messages are hooked by TWO statically-allocated CHudBase-style elements:

1. **The TS master hud element @ `0x100d43cc`** ("CHudTS" — flag HUD + objective HUD + misc; its
   `+0x20` is the thirdperson byte cited in client_display_cvars §1). Its Init = **FUN_10049b80**
   (ends `ecx=0x100cf920 (gHUD); call 0x10020a40 (AddHudElem)`). It hooks **23 messages** through
   per-message cdecl→__thiscall thunks at 0x498a0..0x49b60 (stride 0x20, each `mov ecx,0x100d43cc`):

   | msg | thunk | handler | msg | thunk | handler |
   |---|---|---|---|---|---|
   | TSHealth | 0x498a0 | 0x1004b730 | SrvSett | 0x49a40 | 0x1004b840 |
   | TSArmor | 0x498c0 | 0x1004b920 | TSBTime | 0x49a60 | 0x1004b8e0 |
   | TSState | 0x498e0 | 0x1004b950 | **TSHVoice** | 0x49a80 | **0x1004c180** |
   | TSCash | 0x49900 | 0x1004b980 | **TSFlag** | 0x49aa0 | **0x1004ba50** |
   | TSSpace | 0x49920 | 0x1004b9b0 (→ byte this+0x28) | PTakeDam | 0x49ac0 | 0x1004bcd0 |
   | PwUp | 0x49940 | 0x1004b9e0 | TSFrags | 0x49ae0 | 0x1004bd50 |
   | PwUpTime | 0x49960 | 0x1004bad0 | **TSMBlur** | 0x49b00 | **0x1004bf90** |
   | RoundTime | 0x49980 | 0x1004bb20 | **TSPSys** | 0x49b20 | **0x1004bfd0** |
   | **Objective** | 0x499a0 | **0x1004bb70** | PlInfo | 0x49b40 | 0x1004b7e0 |
   | **ShowObj** | 0x499c0 | **0x1004bc80** | TSFade | 0x49b60 | 0x1004c1e0 |
   | TSMessage | 0x499e0 | 0x1004c0a0 | TSSlowMo | 0x49a00 | 0x1004bef0 |

   Every handler ends `this->m_iFlags(+0x10) |= HUD_ACTIVE`. Its Draw = **FUN_10049ea0**
   (marker sprite when DAT_1018d184; then, if the nohud-ish cvar 0x100df50c == 0: FUN_1004a130
   (gated on this+0x2c), FUN_1004af40, **FUN_1004b0e0 = flag HUD**, then behind
   FirstPersonState(0x1004c250)/0x1004c280 spectator gates → **FUN_10049fc0 = objective HUD**).
   Its VidInit sprite loads (0x49d40–0x49dd3): +0x58 player/health.spr, +0x5c player/kevlar.spr,
   **+0x1434 cursor.spr, +0x1438 compass.spr, +0x143c uparrow.spr, +0x1440 downarrow.spr**,
   +0x64 player/kungfu.spr, +0x68 player/movement.spr, **+0x6c player/flag.spr**.

2. **CHudTSWeaponBar @ `0x100d0ca0`** — second registration block (0x4c9e0..): ClipInfo→0x4c840,
   WStatus→0x4c860, **NextStunt → thunk 0x4c880 → FUN_1004fac0**, Wires→0x4c8a0, CustomWP,
   KFuPower, WeaponInfo, ActItems + client commands slot1/2/3, drop_%i, weapon_%i.

**Wire-read helpers** (engine parse-msg mirror): 0x299b0 BEGIN_READ(buf,size) · 0x29a10 READ_BYTE
(advances 1) · 0x29a50 READ_SHORT (2) · 0x29aa0 READ_LONG (4) · 0x29b00 READ_STRING (static buf
0x100e024c, cap 0x7ff) · 0x29b60 READ_COORD = READ_SHORT × 0.125 (dbl @0x7d4f0).

**Engine funcs** (cl_enginefunc table copy base 0x100d5b28, index×4): 0x5b38 SPR_Set(4) ·
0x5b44 SPR_DrawAdditive(7) · 0x5b54 FillRGBA(11) · 0x5b60 RegisterVariable(14) · 0x5b6c
AddCommand(17) · 0x5b70 HookUserMsg(18) · 0x5b78 ClientCmd(20) · 0x5b80 PlaySoundByName(22) ·
0x5b88 AngleVectors(24) · 0x5bc0/0x5bc4 Cmd_Argc/Argv(38/39) · 0x5bc8 Con_Printf(40) · 0x5bf4
GetLocalPlayer(51) · 0x5bfc GetEntityByIndex(53) · 0x5c00 GetClientTime(54) · 0x5c0c
PM_PointContents(57) · 0x5c34 RandomFloat(67) · 0x5c74 pEfxAPI(83) · 0x5c88 IsSpectateOnly(88).
IEngineStudio copy: 0x100ea7dc PlayerInfo(7), 0x100ea7e0 GetPlayerState(8).
`DAT_100963e8` = game-speed/slow-mo scale · `gHUD @0x100cf920`: +0x24 m_flTime (DAT_100cf944),
+0x30 m_flTimeDelta, +0x40 local view z (DAT_100cf960), +0x44 view-pitch-ish meter angle
(DAT_100cf964). Helpers: FUN_10055aa0(x)=max(x,0.01) · FUN_10055ac0(x)=1.0/max(x,0.01) ·
FUN_10056fa0(d)=d/max(timescale,0.01).

---

## 1. TSMBlur — motion-blur ghost trails ★

**⚠ CORRECTION to client_display_cvars.md §4:** the handler reads **BYTE idx, BYTE level** (two
READ_BYTE calls at 0x4bfa3/0x4bfaa; the `movswl` is just sign handling), NOT "READ_SHORT idx".
Total = 2 bytes = the registered size 2 ✓ = our senders ✓ (ts_player.cpp:393/890/966 UsePowerUp +
TakeDamage rolls, ts_stunt.cpp:62 TS_MotionBlur — all `WRITE_BYTE(entindex); WRITE_BYTE(level 1|2)`).

- **Handler FUN_1004bf90**: `BEGIN_READ; idx=READ_BYTE; lvl=READ_BYTE; FUN_10029680(&DAT_100dff48, idx, lvl)`.
- **Arm FUN_10029680** (this = the 32-entry blur array `DAT_100dff48`, entry stride 0x18 =
  `{byte active; +4 int count; +8 int frames; +0x10 double nextTime}`, entry index == entindex):
  `(int)cl_motionblur == 0 → return`; already active → `frames += 5`; else
  `switch((int)min(lvl, cl_fx->value)) { 1: frames=10; 2: frames=15; default: frames=5 }`,
  `nextTime = gHUD.m_flTime; active=1; count=0`.
- **Per-frame spawner FUN_10029780** — called from **HUD_CreateEntities** (export @0x1bce0 →
  FUN_1001bcd0 → FUN_10034e80, then `ecx=&blurArray; jmp FUN_10029780`). Gate `(int)cl_motionblur != 0`.
  Loop i = 0..31 (so entindex 32 never ghosts — original quirk): entry active && nextTime <= gHUD.m_flTime:
  - NOT in slow-mo (`DAT_100963e8 > 0.99`) → `active = 0` (trail dies instantly at normal speed).
  - else `nextTime = time + 0.03/max(timescale, 0.01)`; `ent = GetEntityByIndex(i)`; needs `ent->model`
    (cl_entity +0xb94; origin +0xb48, angles +0xb54 in this engine build). Then:
    ```
    te = pEfxAPI->CL_TentEntAllocCustom(ent->origin, ent->model, 0, GhostFade@0x29710);
    te->entity.curstate  = *IEngineStudio.GetPlayerState(i-1);   // whole entity_state_t (0x154)
    te->entity.angles    = ent->angles;
    cs.number = i;  cs.rendercolor.r = (byte)i;   // ghost OWNER entindex
    cs.rendercolor.b = 1;                          // ★ THE GHOST MARKER (== the "entstate+0x302"
                                                   //   gate in StudioDrawPlayer FUN_10031ae0)
    cs.rendermode = kRenderTransColor(1); cs.renderamt = 120; cs.renderfx = 17;
    cs.iuser4 = 0; cs.fuser2 = 120.0f;             // fuser2 = float alpha accumulator
    pi = IEngineStudio.PlayerInfo(i-1):            // player remap block → colormap for the ghost
        cs.iuser2 = pi+0x178; cs.iuser1 = pi+0x17c; cs.fuser3 = pi+0x180; cs.fuser4 = pi+0x184; // VERIFY fields
    cs.origin = ent->origin;  (3 dwords te+0x5c zeroed)
    entry->count++;  if (count >= frames) active = 0;
    ```
- **GhostFade FUN_10029710(te, frametime, currenttime)** (exact, from disasm):
  `a = cs.fuser2 − frametime * 255.0 / max(DAT_100963e8, 0.01); cs.fuser2 = a; cs.renderamt = (int)a;
  if (a > 0) te->die = currenttime + 1.0; else { cs.renderamt = 0; te->die = currenttime − 1.0; }`
- **Draw gates** (client_display_cvars §4, 0x31c41): ghosts skipped entirely when not slow-mo;
  YOUR OWN ghosts skipped unless `CL_IsThirdPerson() && (int)cl_motionblur >= 2`.

**Fix spec — new `cl_dll/ts_motionblur.cpp`** (original filename unrecoverable; keep it a plain
module + a `MsgFunc_TSMBlur` hooked from an existing element or a static hook):
1. `struct blurentry_t { int active, count, frames; double nextTime; } g_TSBlur[32];`
2. Handler: exactly the arm logic above (`cl_motionblur`/`cl_fx` gates — cvars already registered).
3. `TS_UpdateMotionBlur()` called from `HUD_CreateEntities` (cl_dll/entity.cpp) — the spawner loop
   above via `gEngfuncs.pEfxAPI->CL_TentEntAllocCustom(org, model, 0, TS_GhostFadeCallback)`;
   timescale = our TSSlowMo-driven slow-amount global (the DAT_100963e8 equivalent).
4. Fade callback: the exact formula above (255 alpha/sec ÷ max(timescale,0.01); die = now±1).
5. Own-ghost hiding: the faithful mechanism is a render-time gate in StudioDrawPlayer keyed on
   `curstate.rendercolor.b == 1 && rendercolor.r == localidx`. If we have no studio hook yet, an
   acceptable interim: don't SPAWN local ghosts unless thirdperson && cl_motionblur>=2 (flag as
   divergence — differs only if the camera toggles mid-trail).
**Implementable NOW** — tempent pipeline + cvars + slow-mo scale all exist. Server senders already ✓.

## 2. NextStunt — the stunt meter ★

- **Handler FUN_1004fac0** (CHudTSWeaponBar): `b = READ_BYTE; m_flStuntTime(+0x34) = b * 0.1f;
  m_iFlags |= HUD_ACTIVE`. 1 byte ✓ = our parked sender (`WRITE_BYTE(round(10*stuntTime))`,
  ts_stunt.cpp TS_SendNextStunt) ✓. **The client never ticks it down** — the server re-sends on
  every change including →0 (our sender already does, at all 8 call sites).
- **Consumer**: the crosshair/dive-arc draw. WeaponBar VidInit (0x4cc04-0x4cc2c) loads
  **+0x158 divestept.spr, +0x15c divestepc.spr, +0x160 divestepb.spr**; meter draw
  **FUN_1004ef50(this,&x,&y)**, called @0x4ea7d from the crosshair draw FUN_1004e9f0:
  - color select: **`m_flStuntTime >= 0.01f → RED (0xc3,0x2d,0x2d)`**, else hud_r/g/b — the whole
    arc goes red while a stunt/dive timer is pending.
  - each of the 3 segments is ALSO forced red when its own check fails: checkers
    FUN_1004eec0/eef0/ef20 read **byte 0x100d43f4 = TS-hud element +0x28 = the TSSpace byte**
    (written by the TSSpace handler @0x4b9cd) — segment lit iff space ≥ 40 / 30 / 20.
  - needle: 2x2 FillRGBA (alpha 100) at angle `clamp(gHUD+0x44 * 3.0, ±90)`° on the arc —
    **our ts_reticle.cpp already implements exactly this** (pitch*3, ±90, R=33 dot).
- **Fix spec**: our `cl_dll/ts_reticle.cpp` already draws the arc + weight tiers + pitch dot.
  1. Hook `"NextStunt"` (handler on CHudTSReticle — our split of the original weaponbar object;
     or on CHudTSWeaponBar with a shared getter): `m_flStuntTime = READ_BYTE() * 0.1f`.
  2. In the arc draw: while `m_flStuntTime > 0.01f`, use (0xc3,0x2d,0x2d) as the segment base
     color (replaces the blue for AVAILABLE tiers; unavailable tiers stay red anyway).
  3. **Unpark `TS_SendNextStunt`** (dlls/ts_stunt.cpp:50 — delete the early return).
**Implementable NOW** — 3 small changes, both libs unaffected (HUD only).

## 3. TSPSys — particle-system trigger (only subtype 3 = wind exists client-side)

- **Handler FUN_1004bfd0**: `t = READ_BYTE; if (t != 3) return 1;` — **all other subtypes are
  ignored by the original client** (our DEAD SV_UpdateClient subtype-1 pings would be no-ops ✓).
  Subtype 3 wire: **BYTE 3, LONG seed(0..30000), LONG yawDeg, COORD strength** — byte-for-byte
  what our `dlls/ts_emitter.cpp` CTSWind::BlowThink sends ✓.
- State written (the client wind block): `DAT_10189438 = seed` ·
  `DAT_10189454[3] and DAT_10189460[3] = AngleVectors({0, yaw, 0}).forward` (current + target set
  equal on receive) · `DAT_10189440 / DAT_10189444 = strength` (current + target) ·
  `DAT_1018946c = GetClientTime()`.
- **Consumer**: the TSP particle system's per-frame wind update (reads/lerps the block; code region
  ~FUN@0x10051aa0 inside the particle manager) — **un-reconstructed TSP internals; boundary here.**
- **Fix spec**: add `MsgFunc_TSPSys` in `cl_dll/ts_particles.cpp`: read subtype; subtype 3 → store
  `{seed, windDir(current/target), windStrength(current/target), changeTime}` in the TSP module;
  ignore other subtypes. Consumption (applying wind to particles) lands with the TSP wind pass —
  **handler NOW, consumer DEFERRED** (mark the globals as the seam).

## 4. TSFlag — flag/objective-carry HUD

- **Handler FUN_1004ba50**: 4×READ_BYTE: `action (0 base-spawn / 1 pickup / 2 drop), flagID,
  teamPlus1, carrier`. Guard `0 < flagID < 16` (correct &&). State on the TS-hud element:
  array @+0x146c, **16 entries stride 0xc `{int action; int carrier; int team}`**, indexed by
  flagID; `carrier` stored only when action==1 (else 0); `+0x1468 = 16` (entry ceiling; the draw
  trims it to lastActive+1). Our senders agree ✓ (ts_flag.cpp / ts_player.cpp:478 / 534 /
  TS_objectives.cpp:852 — same 4-byte order).
- **Consumer FUN_1004b0e0** (from element Draw when nohud==0): for each entry with
  **action != 0** (i.e. only flags AWAY from base) and 1 ≤ team ≤ 4:
  - flag icon = **sprites/player/flag.spr** (+0x6c), `SPR_Set(hud r,g,b)` + SPR_DrawAdditive at
    `x = ScreenWidth − 0x38, y = team*0x30 + 0x28` (one row per team, right edge).
  - carrier name (client player-info array DAT_1018fbd8 stride 0x14, name ptr +0; strncpy 24)
    drawn left-aligned against the icon (x = iconX − stringwidth − 0x20-ish).
  - if carrier == GetLocalPlayer()->index: centered text **"Take the Briefcase to the base!"**
    at y=0x140, alpha ≈ hudbase+0x23 clamp 255.
  - blink state: +0x1530/+0x1534 countdowns (reset to 0.1/0.5 s, toggling bools +0x152c/+0x152d,
    decrement `m_flTimeDelta / max(timescale,0.01)`) — updated here; exact visual coupling was
    elided by the decompiler (VERIFY at playtest: likely icon/text blink).
- **Fix spec**: new element or extend `cl_dll/ts_status.cpp`: `MsgFunc_TSFlag` + the entry array +
  the Draw above (flag.spr ships in the ts content). Use our g_PlayerInfoList for carrier names.
**Implementable NOW.**

## 5. Objective — HUD objective entries

- **Handler FUN_1004bb70**: `COORD x, COORD y, COORD z, BYTE idx, STRING label` (label buffered to
  64) — matches our sender `CTSGameRules::SendObjectiveToHud` ✓. **idx == 0 clears the whole
  table** (0x280 dwords @+0xa30) and maxIdx=0 — the server always sends the list in order, so
  entry 0 doubles as the reset. Entry: array @+0xa30, **32 entries stride 0x50
  `{byte visible; float x,y,z; char label[0x40]}`**; sets visible=1; `+0x1430 maxIdx = max(maxIdx, idx)`.
  ⚠ original bounds check is `(idx > -1) || (idx < 0x20)` — **always true (original `||` bug)**;
  write ours as `&&` with a provenance comment (drop-in identical for well-formed servers).
- **Consumer FUN_10049fc0** (from element Draw, behind !nohud + first-person/spectator gates):
  per entry `visible && label[0]`: WorldToScreen (FUN_100579c0 → DAT_1018a5b0/DAT_1018a4cc, −1 =
  offscreen): icon **compass.spr** (+0x1438) SPR_Set(100,100,0xcd) additive at (sx−16, sy−16);
  vertical cue vs **gHUD+0x40 (local view z) ± 72.0**: entry higher → **uparrow.spr** at
  (sx−16, sy−0x30), lower → **downarrow.spr** at (sx−16, sy+0x10); label text at (sx+0x10, sy−8),
  color (100,100,0xcd).
- **Fix spec**: `MsgFunc_Objective` + the 32×0x50 table + the draw (same element as §4/§6;
  sprites compass/uparrow/downarrow ship with ts). Local view z: use our view origin z.
**Implementable NOW.**

## 6. ShowObj — objective visibility toggle

- **Handler FUN_1004bc80**: `BYTE idx, BYTE show` → `entry[idx].visible = (show != 0)` (same
  array as §5; same always-true `||` bounds quirk — use `&&`). Our sender ✓
  (ShowObjectiveOnHud: idx, 0/1). Rides §5's table/draw. **Implementable NOW.**

## 7. TSHVoice — capture-taunt voice

- **Handler FUN_1004c180**: `BYTE v`: `0 → PlaySoundByName("misc/successful.wav", 1.0f)`;
  `1 → PlaySoundByName("misc/failed.wav", 1.0f)`; anything else → nothing (still sets HUD_ACTIVE).
- Our sender ✓ (TS_objectives.cpp:952 — 0 = your team captured, 1 = an enemy team did; matches
  successful/failed semantics).
- **Fix spec**: 4-line `MsgFunc_TSHVoice` (ts_status.cpp or wherever §4-6 land) via
  `gEngfuncs.pfnPlaySoundByName`. **Implementable NOW** (wavs are ts content).

## 8. PTakeDam blood tail (cl_gore consumer) — what health.cpp:146 must become

**Placement correction:** the ORIGINAL hooks PTakeDam on the TS-hud element (handler
**FUN_1004bcd0**), and it does **NOTHING but blood**: 6×READ_COORD (hit point, hit dir) +
READ_BYTE enc → `FUN_10048840(point, dir, enc)`. It does NOT feed a pain-direction indicator —
**our `CHudHealth::MsgFunc_PTakeDam`'s `CalcDamageDirection(vecPoint)` call is an invention**
(remove it, or flag it as deliberate divergence; the original TS has no Damage-msg pain arrows).

**FUN_10048840(float* point, float* dir, int enc)** — the blood spray, on the TSP particle manager
(`DAT_1018bde8` = our cl_dll/ts_particles.cpp system):
1. Visibility gates: FUN_100570b0(point) + FUN_10057270/FUN_100572a0 (view-relative cull) — skip if unseen.
2. `bloodvel = dir * −1.0` (inverted hit dir). `armored = enc > 127; if (armored) enc −= 128;
   base = enc * 2` (server encodes dmg*0.5 → base ≈ original damage ✓ our dlls/player.cpp:390 sender).
3. If armored (bit7): jitter bloodvel by RandomFloat per component, normalize, and spawn a
   **gun_smoke_add.spr dust puff** (FUN_100464f0, count 5, near point + dir*6 — exact params
   decompiler-elided, VERIFY) instead of extra blood; base is further randomized+halved (elided; see
   client_display_cvars §3 note "(base−128)*2 then +random and halved").
4. `count` from **cl_gore** (client_display_cvars §3 table, confirmed): 0→0 · 1→base*0.5 · 2→base ·
   3→base*2.5 · 4→base*5 · 5..10→base*10 · >10→base*min(v,33).
5. Template particle (FUN_1004ffe0; texture `sprites/bloodfrag2.spr` via FUN_100509d0): dword-idx
   fields: [0x45]s=texid · byte@0=1 · byte@4=3 · [0x34]=0.5f · [0x37]=1.0f · [0x38]=0 · [0x3f]=0 ·
   **[0x42]=clamp(base*0.075, 1.0..5.0)** (size) · [0x48]=2 · [0x4a]=2.0f.
6. Per-particle loop (skip while PM_PointContents == CONTENTS_SOLID(−2); spawn
   FUN_10050b60(&mgr, p, 0, cb@0x48710)): [0x39]=0.075f, [0x36]=RandomFloat;
   **first particle: velocity = dir * 32.0, pos = point** (the main spurt);
   subsequent: velocity = bloodvel * RandomFloat (+xy jitter), and the spawn pos WALKS a streak:
   `pos += size*bloodvel*0.3` per step with an accumulating droop `acc += size*0.1; z −= acc/count`.
- **Fix spec**: implement `TSP_BloodSpray(Vector point, Vector dir, int encByte)` in
  `cl_dll/ts_particles.cpp` honoring 2/4/6 (count table + first-particle dir*32 + streak walk +
  solid-contents skip + armored smoke-puff variant), call it from MsgFunc_PTakeDam, delete the
  CalcDamageDirection invention. **Implementable NOW to first order** if our TSP layer can spawn
  textured fading particles (TSP_Smoke exists); the exact template-field mapping (step 5 offsets)
  is the TSP-struct seam — map what our struct supports, VERIFY-tag the rest.

## 9. specmode + special — mechanism swap (client-side commands, NOT server ClientCommand)

**Ground truth:** `ts_i386.so` contains **NO "specmode"/"special"/"_special" strings** — the
original TS SERVER removed the stock branches. Both are CLIENT commands in the original client:

- **specmode**: registered in the spectator init @0x22cd5 — `pfnAddCommand("specmode", cb 0x10022c10)`,
  immediately after `pfnRegisterVariable("cl_drawnames", "1")` (→ note for the missing-cvars item:
  cl_drawnames default "1" lives in the spectator init). Callback 0x10022c10:
  ```
  if (!IsSpectateOnly())      { Con_Printf("specmode only in HLTV mode available.\n"); return; }
  if (Cmd_Argc() < 2)         { Con_Printf("usage:  specmode < modenumber >\n"); return; }
  CHudSpectator(0x100cfff8)::SetModes(atoi(Cmd_Argv(1)), 0);   // FUN_100234d0
  ```
  The original client does **NOT forward specmode to the server** for non-HLTV (it just errors).
  **Fixes:** (a) remove the server branch `dlls/client.cpp:788` ("specmode"); (b) register the
  client command `"specmode"` (exact name) in cl_dll/hud_spectator.cpp Init with the behavior
  above (our SetModes equivalent); (c) our `hud_spectator.cpp:1152` currently does
  `pfnServerCmd("specmode %i")` when !IsSpectateOnly — the original never sends that (and original
  servers don't handle it) → drop the forward, keep mode changes HLTV-local. The SDK-added
  `spec_mode`/`spec_*` names remain EXTRA client cmds (parity_sweep #7 strip list).
- **special**: registered @0x20213 in the stock viewport/hud init (same block as
  +commandmenu/-commandmenu/ForceCloseCommandMenu) → cb 0x1001fe90 → gViewPort(0x100d5cb0)→
  InputPlayerSpecial (0x637a0): TFC gamemodes {8,9} → local class menu (0x37), else
  `pfnClientCmd("_special")` (forwards to the server, which IGNORES it — dead TFC leftover).
  **Our client already matches byte-for-byte** (cl_dll/hud.cpp:284 HOOK_COMMAND("special",
  InputPlayerSpecial) → vgui_TeamFortressViewport.cpp:1112 EngineClientCmd("_special")).
  **Fix:** only remove `special` from the server's leftover accept-branch (dlls/client.cpp:804
  block) so it falls through to the unknown-command tail like the original. (Same removal applies
  to the `+alt1/-alt1/+break/-break` neighbors per parity_sweep #3 — engine +/- buttons, original
  server has no strings for them either.)

## Summary — priority order

| # | msg | wire (client reads) | consumer | fix locus (ours) | now? |
|---|---|---|---|---|---|
| 1 | TSMBlur | BYTE idx, BYTE lvl (**not** SHORT) | tempent ghosts from HUD_CreateEntities | new ts_motionblur.cpp + entity.cpp | ✅ |
| 2 | NextStunt | BYTE ×0.1 → m_flStuntTime | dive-arc turns red ≥0.01; no local decay | ts_reticle.cpp + unpark TS_SendNextStunt | ✅ |
| 8 | PTakeDam | 6×COORD + BYTE | TSP blood spray only (no pain dir!) | health.cpp handler → ts_particles.cpp TSP_BloodSpray | ✅ (approx) |
| 4 | TSFlag | 4×BYTE (action,id,team+1,carrier) | per-team flag row, right edge; carrier text | ts_status.cpp (or new) MsgFunc+Draw | ✅ |
| 5 | Objective | 3×COORD, BYTE idx, STRING | world marker compass + up/down arrows + label; idx0 = reset | same element as §4 | ✅ |
| 6 | ShowObj | BYTE idx, BYTE show | visible bit on §5's table | same | ✅ |
| 7 | TSHVoice | BYTE 0/1 | misc/successful.wav / misc/failed.wav | tiny MsgFunc | ✅ |
| 3 | TSPSys | BYTE 3, LONG seed, LONG yaw, COORD strength (others ignored) | TSP wind lerp (un-reconstructed) | ts_particles.cpp handler now; wind pass later | handler ✅ / consumer ⏳ |
| 9 | specmode/special | client cmds | HLTV SetModes / viewport _special | client.cpp branch removals + hud_spectator registration | ✅ |

Verification pointers: all addresses above are client.dll RVAs (+0x10000000 for Ghidra).
Handlers decompiled to scratchpad (cont94 session); registration blocks at 0x49b80–0x49d0d
(TS-hud element) and 0x4c9c0–0x4ca60 (weaponbar). Constants: 0.1@0x7a880 · 0.01@0x7a920 ·
255.0@0x7cae0 · −1.0@0x7d4d8 · 0.075@0x7d754 · 32.0@0x7a94c · 0.3@0x7a8b0 · 0.1@0x7a84c ·
72.0@0x7d588 · 0.99@0x7d4e8 · dbl 3.0@0x7a8e8 · dbl ±90@0x7d7c0/0x7d7b8 · deg2rad@0x7d518.
