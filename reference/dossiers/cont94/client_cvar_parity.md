# cont94 — client cvar parity: the 10 MISSING + 21 EXTRA registrations

READ-ONLY RE dossier (branch restructure-source-parity). Companion to
`client_display_cvars.md` (whose §0 covers the InitInput registration run 0x10026f00–0x1002721f).
Source of truth = `reference/client.dll` (PE imagebase 0x10000000; **file offset == RVA** for every
range cited). Method: name string → `push imm32` scan → `dc.sh client 0x<rva>` decompile of the
containing function; cvar_t* globals traced by imm32 scan; value reads are `cvar_t.value` @ +0xc.

Engine-table decode used throughout (base of the copied `cl_enginefunc_t` = **0x100d5b28**):
`0x100d5b60`=pfnRegisterVariable(#14) · `0x100d5b64`=pfnGetCvarFloat(#15) · `0x100d5b6c`=pfnAddCommand(#17) ·
`0x100d5b70`=pfnHookUserMsg(#18) · `0x100d5ba8/ac`=GetWindowCenterX/Y · `0x100d5bb0/b4`=Get/SetViewAngles ·
`0x100d5bb8`=**GetMaxClients(#36)** · `0x100d5bf0`=IsNoClipping(#50) · `0x100d5bf4`=GetLocalPlayer(#51) ·
`0x100d5c48`=pfnGetCvarPointer(#72) · `0x100d5c50`=pfnGetLevelName(#74) · `0x100d5c68`=COM_ParseFile-equiv.
`DAT_100ea860` = the saved "is hardware GL" check fn (same pointer the crosshair fn 0x4ecf5 uses —
`client_display_cvars.md §6`'s `IsHardwareGL()`).

The MISSING/EXTRA lists are `reference/coverage/REPORT.md` "Client cvars" (11 missing, 21 extra);
`cl_drawnames` is already DONE (registered in `cl_dll/hud_spectator.cpp:206`) → 10 remain.

---

# TASK A — the 10 missing registrations

## A1. gl_fog / gl_fog_red / gl_fog_green / gl_fog_blue / gl_fog_start / gl_fog_end

### Registration (all six in **CHud::Init = FUN_10020170**, right after `zoom_sensitivity_ratio`)

| order | cvar | default | flags | cvar_t* stored |
|---|---|---|---|---|
| 1 | gl_fog | **"0"** | FCVAR_ARCHIVE | `DAT_100df19c` (0x20380) — **never read back**; all reads are by-name |
| 2 | gl_fog_start | "0" | 0 | discarded |
| 3 | gl_fog_end | "2048" | 0 | discarded |
| 4 | gl_fog_red | ".5" | 0 | discarded |
| 5 | gl_fog_green | ".5" | 0 | discarded |
| 6 | gl_fog_blue | ".5" | 0 | discarded |

Registration sites 0x20372–0x203d4. Full original order in CHud::Init: `hud_classautokill`,
`hud_takesshots`, `r_paintball`, `zoom_sensitivity_ratio`, **gl_fog, gl_fog_start, gl_fog_end,
gl_fog_red, gl_fog_green, gl_fog_blue**, `default_fov`, `hud_capturemouse`, then
`cl_lw` via pfnGetCvarPointer.

### Consumer — FUN_10056c20 (0x56c20, 417 B): the per-frame fog-enable pass

Sole caller: **0x69e29 = the tail of V_CalcRefdef** (export V_CalcRefdef @0x69b90; V_Init @0x69e40)
— fog is (re)applied every refdef. All six cvars are read **by name via pfnGetCvarFloat** each call.

```c
void TS_ApplyFog(void)   // FUN_10056c20
{
    // ---- path 1: USER-override fog — SINGLE PLAYER ONLY ----
    if (gEngfuncs.GetMaxClients() == 1                      // 0x56c2a
        && gEngfuncs.pfnGetCvarFloat("gl_fog") == 2.0f) {   // 0x56c34; const 0x1007a924 = 2.0f
        col[0] = pfnGetCvarFloat("gl_fog_red");             // 0x56c53   (defaults .5/.5/.5)
        col[1] = pfnGetCvarFloat("gl_fog_green");           // 0x56c62
        col[2] = pfnGetCvarFloat("gl_fog_blue");            // 0x56c71
        glFogi (GL_FOG_MODE /*0xB65*/, GL_LINEAR /*0x2601*/);
        glFogfv(GL_FOG_COLOR /*0xB66*/, col);
        glFogf (GL_FOG_DENSITY /*0xB62*/, 1.0f);
        glHint (GL_FOG_HINT /*0xC54*/, GL_NICEST /*0x1102*/);
        glFogf (GL_FOG_START /*0xB63*/, pfnGetCvarFloat("gl_fog_start"));   // 0x56ccd
        glFogf (GL_FOG_END   /*0xB64*/, pfnGetCvarFloat("gl_fog_end"));     // 0x56ce2
        glEnable(GL_FOG /*0xB60*/);
        return;
    }
    // ---- path 2: MAP fog (the ts_fog entity) ----
    if (*(byte*)0x101893b8 == 0) return;                    // 0x56cf5 — map-fog active flag
    if (pfnGetCvarFloat("gl_fog") == 0.0f) return;          // 0x56d02 — user kill-switch (default "0"!)
    if (IsHardwareGL() != 1) return;                        // 0x56d26 via DAT_100ea860 — GL only
    // same GL sequence, but color = floats @0x101893c4/c8/cc, start = @0x101893bc (near),
    // end = @0x101893c0 (far) — the parsed ts_fog values.
}
```

Semantics: **gl_fog 0 (default) = fog OFF (even on fog maps); 1 = honor the map's ts_fog;
2 = (single-player only) force user fog from gl_fog_red/green/blue/start/end.** The 2.0
compare is exact; the map-fog gate is a `!= 0` float test.

### Where the map fog comes from — NOT a user message; the client parses the BSP entity lump

- **`CTS_CL_Emitter::ProcessEnts` = FUN_1003a5d0** (0x3a5d0, 533 B — the original's own name, from
  its error strings 0x1009520c/0x100951cc). It walks the entity text with the engine parse fn
  (`*0x100d5c68`), parses each `{…}` block, and dispatches by classname: `ts_particle` (→ the
  particle-def table 0x10108bb8), `ts_emitter` (→ 0x101663b8 + the active-emitter list 0x1018abc8),
  and **`ts_fog`** (string 0x10095204) → **FUN_1003a7f0(&parsed)**, which `rep movsl` copies **31
  dwords** into the fog state at **0x101893b8** and then sets its first byte = 1 (active).
  Struct layout (matches the FGD keys of `ts_fog`): `+0` active byte, `+4` **near** (start),
  `+8` **far** (end), `+0xc/+0x10/+0x14` **color r/g/b** (floats).
- Map-change plumbing: **FUN_1003ab40** (0x3ab40, the per-frame emitter update) compares
  `pfnGetLevelName()` against the cached name at 0x10108b38; on change it zeroes the emitter
  tables AND the 31-dword fog struct (`rep stos` @0x3abbd) and re-runs the loader (FUN_1003a960 →
  ProcessEnts). So fog appears iff the CURRENT map's entity lump contains a `ts_fog`.
- Server side: the original `CTSFog::Spawn` just removes itself — **our `dlls/ts_emitter.cpp`
  CTSFog already matches** (lines 182–193). There is no fog wire message; nothing server-side to do.

### Our client today

`cl_dll` has **no fog code at all** (no glFog / GL_FOG anywhere) and no client-side entity-lump
parser (the whole CTS_CL_Emitter subsystem — ts_particle/ts_emitter/ts_fog — is un-reconstructed).
The comment at `cl_dll/ts_hud.cpp:46` **wrongly claims** gl_fog* (and cl_thirdperson) are
"registered by the engine/base client" — they are mod cvars; fix the comment.

### Fix spec

1. **Register now** (closes the coverage gap): in `cl_dll/hud.cpp` `CHud::Init`, immediately after
   the `zoom_sensitivity_ratio` CVAR_CREATE (line ~312), mirroring the original order:
   ```c
   CVAR_CREATE("gl_fog", "0", FCVAR_ARCHIVE);
   gEngfuncs.pfnRegisterVariable("gl_fog_start", "0", 0);
   gEngfuncs.pfnRegisterVariable("gl_fog_end", "2048", 0);
   gEngfuncs.pfnRegisterVariable("gl_fog_red", ".5", 0);
   gEngfuncs.pfnRegisterVariable("gl_fog_green", ".5", 0);
   gEngfuncs.pfnRegisterVariable("gl_fog_blue", ".5", 0);
   ```
   (Only gl_fog is ARCHIVE. Keep the returned pointers unused — the original reads by name.)
2. Fix the stale comment at `cl_dll/ts_hud.cpp:46`.
3. **Documented seam** — the consumer is the un-reconstructed client fog pass: when the
   CTS_CL_Emitter client subsystem (or a first fog-only slice) is built, implement FUN_10056c20
   exactly as above and call it from the V_CalcRefdef tail, plus the level-change parse of
   `ts_fog` (near/far/color) from the entity lump. The fog-only slice is small and independent of
   the particle/emitter tables: parse-on-map-change + the 40-line GL fn. `glDisable(GL_FOG)` end-of
   -frame lives in the FX render root FUN_10056dd0 (see `client_display_cvars.md §5`) — pair them.

## A2. cl_thirdperson

### Registration

**InitInput = FUN_10026b30** (the 0x10026f00-block; site 0x2719a, store 0x271b1):
`cl_thirdperson` default **"0"**, **FCVAR_ARCHIVE**, cvar_t* → `DAT_100df544`.

### Consumer — FUN_10065250 = MsgFunc_Spectator (the ONLY reader: 0x652c4 / 0x652db)

Hook chain: CHud::Init hooks **"Spectator"** → thunk FUN_10020110 → call 0x20129 → FUN_10065250.
Wire (matches our cont.84 finding): BYTE playerIndex (validated 1..**64**), BYTE flag.

```c
int MsgFunc_Spectator(...)             // FUN_10065250
{
    idx = READ_BYTE(); if (idx < 1 || idx > 64) return 1;
    val = READ_BYTE();
    g_IsSpectator[idx] = val;                          // int array DAT_101900ec
    if (GetLocalPlayer()->index == idx) {
        DAT_100d4401 = (val == 1);                     // local "spectating" flag
        if (val == 1) {                                // ENTERING spectate:
            if (CL_IsThirdPerson()) ToggleThirdPerson();   // FUN_1004fe00 — force FIRST person
            FUN_10036670(&DAT_100d1460);               // (unidentified reset — camera/interp)
        } else {                                       // LEAVING spectate (spawning):
            if (*(int*)&cl_thirdperson->value == 0x3f800000) {   // == 1.0f BIT-exact
                if (!CL_IsThirdPerson()) ToggleThirdPerson();    //   force THIRD person
            } else if (cl_thirdperson->value == 0.0f) {          // == 0.0f
                if (CL_IsThirdPerson()) ToggleThirdPerson();     //   force FIRST person
            }                                          // any other value: leave camera as-is
        }
        FUN_10004600();                                // (unidentified — runs on local either way)
    }
    ((byte*)&DAT_1018d128)[idx*2] = 0;                 // reset the per-player laser byte
    ((byte*)&DAT_101901f4)[idx*0x20 + 1] = val;        // extra-player-info spectator byte
    return 1;
}
```

Semantics: **cl_thirdperson = your preferred camera on (re)spawn/unspectate — 1 forces third
person, 0 (default) forces first person, anything else keeps the current camera.** Entering
spectate always forces first person. Distinct from `cl_thirdpersonswitch` (bare-hands auto-switch,
`client_display_cvars.md §8`) and from the never-registered `cl_camtype`-driven cam internals.

### Fix spec

1. Register in `cl_dll/ts_hud.cpp` `TSHud_RegisterCvars()` (next to `cl_thirdpersonswitch`,
   line ~61): `CVAR_CREATE("cl_thirdperson", "0", FCVAR_ARCHIVE);` — keep a
   `pfnGetCvarPointer`/static for the consumer. (Original file-provenance is input.cpp's
   InitInput; ts_hud.cpp is where our TS block lives — acceptable, note the provenance.)
2. Consume in `cl_dll/ts_spectate.cpp`. Our `CHudSpectate::MsgFunc_Spectator` (line 65)
   deliberately defers local-state consequences to the per-frame `Draw()` edge detector
   (parse-time local index is unreliable during connect — round-7c/7d). Apply the camera logic on
   those SAME edges in `Draw()` (behaviorally equivalent to the original's parse-time apply,
   which re-runs on every local Spectator message):
   - entering spectate (`spec && m_iState == 0` branch): `if (CL_IsThirdPerson()) CAM_ToFirstPerson();`
   - entering play (`!spec && m_iState != 0` branch):
     ```c
     float v = s_cl_thirdperson->value;
     if (v == 1.0f)      { if (0 == CL_IsThirdPerson()) CAM_ToThirdPerson(); }
     else if (v == 0.0f) { if (0 != CL_IsThirdPerson()) CAM_ToFirstPerson(); }
     ```
     Exact tests: **only exactly 1.0 forces third, only exactly 0.0 forces first** (e.g. 2 = keep).
3. FUN_10036670/FUN_10004600 remain unidentified — do NOT invent equivalents; leave a VERIFY
   note (candidates: interp/camera-save reset; trace live if a glitch shows on spectate entry).

## A3. m_accel / m_accelexp / m_accelmax

### Registration

**FUN_10028320** (0x28320) = the original's mouse/joystick startup (inputw32 provenance), called
from InitInput. It registers `m_filter "0"`, `sensitivity "3"` (both ARCHIVE), then:

| cvar | default | flags | cvar_t* | site |
|---|---|---|---|---|
| m_accel | "0" | FCVAR_ARCHIVE | `DAT_100df6a0` | 0x28351 |
| m_accelexp | **"1.2"** | FCVAR_ARCHIVE | `DAT_100df6f4` | 0x28368 |
| m_accelmax | "0" | FCVAR_ARCHIVE | `DAT_100df628` | 0x2837f |

then the joystick block (`joystick`, `joyname`, `joyadvanced`, `joyadvaxis[xyzruv]`,
`joy*threshold`, `joy*sensitivity`, `joywwhack1/2`, `joyadvancedupdate`). **This is exactly the
slot where the stock SDK registers `m_customaccel/_scale/_max/_exponent` (+ HL25's
`m_mousethread_sleep`/`joysupported`) — the original REPLACED the stock custom-accel mechanism
with m_accel*.** Task B's m_customaccel* fix is therefore this swap.

### Consumer — FUN_100274d0 = IN_MouseMove (the ONLY reader; ptr loads 0x276e3/0x276ea/0x276f8)

Structure (decompile + disasm 0x27690–0x277f0):

1. Gates: not camera/menu (`DAT_100df394==0 && DAT_100df714==0 && DAT_100d5b20==0`), GetCursorPos,
   deltas vs GetWindowCenterX/Y → **raw int deltas ix, iy**.
2. `m_filter`: off → `mx=(float)ix` ; on → `mx=(ix+prev_x)*0.5f` (same for y). (The filter branch
   carries a decompiler-noise overflow guard; behaviorally it is the stock average.)
3. Cursor-position accumulator for the in-game menus (clamped to ScreenInfo w/h) — unrelated.
4. If mouse active (`DAT_1018d184==0`):
   ```c
   float zoomSens = gHUD.GetSensitivity();       // FUN_10020a90(gHUD=0x100cf920)
   if (zoomSens == 0.0f) {                       // NOT zoomed → the accel path
       // per axis (X shown; Y identical with iy/my):
       float a = pow(fabs(mx), m_accelexp->value) * m_accel->value;   // pow = __CIpow @0x6c490
       if (m_accelmax->value != 0.0f && m_accelmax->value <= a)
           a = m_accelmax->value;                                     // clamp only when max != 0
       mx += (ix > 0 ? +1.0f : -1.0f) * a;       // sign from the RAW INT delta (ix>=1 → +1)
       ...same for my...
       mx *= sensitivity->value;                 // sensitivity applied AFTER accel
       my *= sensitivity->value;
   } else {                                      // zoomed: scale only — NO accel, NO sensitivity cvar
       mx *= zoomSens;  my *= zoomSens;
   }
   viewangles[YAW] -= mx * m_yaw->value;         // no +strafe sidemove branch for X (the original
                                                 //   registers no +mlook/+strafe — freelook always on)
   if (!(DAT_100df530 & 1)) {                    // (vestigial strafe-state test — cannot be set)
       viewangles[PITCH] += my * m_pitch->value;
       clamp PITCH to [-89, +89];                // HARDCODED ±89 — no cl_pitchup/cl_pitchdown
   } else {  (IsNoClipping ? cmd->upmove : cmd->forwardmove) -= my * m_forward->value;  }
   ```
5. If (ix||iy) → recenter cursor.

Key parity deltas vs our stock `IN_ScaleMouse` (`cl_dll/inputw32.cpp:490`):
- **Formula is per-axis ADDITIVE** (`mx += sign*min(pow(|mx|,exp)*m_accel, max)`), not the stock
  multiplicative `(pow(dist,exp)*scale + sens)`.
- **fabs of the FILTERED float** feeds pow; **the sign comes from the raw int delta** (`>0 → +1`,
  `<=0 → -1`; a zero delta contributes pow(0,exp)*m_accel and m_accel=0 makes the whole term 0 —
  no explicit enable gate exists or is needed).
- **Zoomed (gHUD.GetSensitivity()!=0): no accel and no `sensitivity` multiply at all** (stock
  folds zoom sens into the same formula).
- m_accelmax clamps only when non-zero (default "0" = uncapped).

### Fix spec — `cl_dll/inputw32.cpp`

1. In `IN_StartupMouse`/init (lines ~1188–1196): **delete** the four `m_customaccel*`
   registrations and **add** (same spot, matching original order after `m_filter`/`sensitivity`):
   ```c
   m_accel    = gEngfuncs.pfnRegisterVariable("m_accel",    "0",   FCVAR_ARCHIVE);
   m_accelexp = gEngfuncs.pfnRegisterVariable("m_accelexp", "1.2", FCVAR_ARCHIVE);
   m_accelmax = gEngfuncs.pfnRegisterVariable("m_accelmax", "0",   FCVAR_ARCHIVE);
   ```
2. Replace `IN_ScaleMouse` (line 490) with the original model. It needs the raw int deltas —
   change the seam to `IN_ScaleMouse(int ix, int iy, float* x, float* y)` (sole caller: line 613
   in IN_MouseMove, where `pos.x/pos.y` are in scope), body:
   ```c
   float zoomSens = gHUD.GetSensitivity();
   if (zoomSens != 0) { *x *= zoomSens; *y *= zoomSens; return; }
   float ax = powf(fabsf(*x), m_accelexp->value) * m_accel->value;
   if (m_accelmax->value != 0 && m_accelmax->value <= ax) ax = m_accelmax->value;
   *x += (ix > 0 ? 1.0f : -1.0f) * ax;
   float ay = powf(fabsf(*y), m_accelexp->value) * m_accel->value;
   if (m_accelmax->value != 0 && m_accelmax->value <= ay) ay = m_accelmax->value;
   *y += (iy > 0 ? 1.0f : -1.0f) * ay;
   *x *= sensitivity->value;  *y *= sensitivity->value;
   ```
   (Also called with accumulated deltas on the mousethread path if kept — pass those ints.)
3. Ties into Task B: the ±89 pitch clamp hardcode (cl_pitchup/down removal, B§) belongs to the
   same functions. The m_customaccel==2 mode's m_yaw/m_pitch scaling disappears with the swap.

---

# TASK B — the 21 EXTRA registrations

All 21 confirmed **absent from reference/client.dll** (zero string hits each). Cross-check for
engine dependence: grep of `xash3d-fwgs/engine` finds **no by-name lookups of any of them** →
nothing is KEEP-JUSTIFIED for engine reasons. Classes: **SWAP** (replaced by an original
mechanism), **SAFE-REMOVE** (no live reader / debug-only), **NEEDS-GUARD** (live readers — remove
registration AND rework the reader as specified).

| # | cvar | ours (reg site) | readers | class → action |
|---|---|---|---|---|
| 1 | m_customaccel | inputw32.cpp:1188 | IN_ScaleMouse only (499–519) | **SWAP** — deleted by A3 |
| 2 | m_customaccel_scale | inputw32.cpp:1189 | same | **SWAP** (A3) |
| 3 | m_customaccel_max | inputw32.cpp:1190 | same | **SWAP** (A3) |
| 4 | m_customaccel_exponent | inputw32.cpp:1191 | same | **SWAP** (A3) |
| 5 | m_mousethread_sleep | inputw32.cpp:1196 | 230 (thread sleep ms), 1198 (guard) | **NEEDS-GUARD** — remove registration; the reader is HL25's optional `-mousethread` path. Either delete the mousethread feature (original has none) or hardcode the 10 ms sleep. |
| 6 | joysupported | inputw32.cpp:1176 | **none** (`joy_supported` never dereferenced) | **SAFE-REMOVE** — delete registration + the dead `joy_supported` decl (line 164). The real `joystick`/`joy*` cvars ARE original — keep them. |
| 7 | cl_pitchup | input.cpp:1022 | input.cpp:641–644 (CL_AdjustAngles); inputw32.cpp:624–627 (IN_MouseMove), 1134–1137 (IN_JoyMove) | **NEEDS-GUARD** — original registers neither and **hardcodes ±89** (verified in FUN_100274d0's clamp). Remove both registrations + externs; replace all three read pairs with literal `89.0f`. |
| 8 | cl_pitchdown | input.cpp:1023 | same sites | same as #7 |
| 9 | cl_rollangle | hud.cpp:318 | view.cpp:421 (`V_CalcRoll(viewentity->angles, simvel, cl_rollangle, cl_rollspeed)`) | **NEEDS-GUARD** — HL25 drift: default "2.0" ADDS view roll the original doesn't have. Revert the call to the SDK-2.3 form `pparams->movevars->rollangle / ->rollspeed` (server default 0 → no roll, the original's visible behavior). Remove hud.cpp regs + externs (view.cpp:63–64, hud.h). VERIFY (low risk): confirm FUN_10067890 sources roll from movevars when that region is next in Ghidra. |
| 10 | cl_rollspeed | hud.cpp:319 | view.cpp:421 | with #9 |
| 11 | cl_bobtilt | hud.cpp:320 | view.cpp:956 (`if (cl_bobtilt->value)` extra viewmodel tilt) | **SAFE-REMOVE** — HL25 addition, default "0" = branch already dead. Delete registration + extern + the gated branch (keep the else/default path). |
| 12 | cl_chasedist | view.cpp:2056 | view.cpp:1591 (`V_GetChaseOrigin(..., cl_chasedist->value, ...)` spectator chase) | **NEEDS-GUARD** — reader is the SDK spectator refdef. Remove registration; replace the read with a file-local constant 112.0f + `// VERIFY: original chase distance un-RE'd (its spectator refdef hardcodes it; RE with the spectator subsystem)`. |
| 13 | cl_autowepswitch | hud.cpp:313 (ARCHIVE\|USERINFO, ptr discarded) | client: none. SERVER: dlls/player.cpp:5146 `InfoKeyValue(..., "cl_autowepswitch")` | **NEEDS-GUARD (cross-side)** — `ts_i386.so` contains NO `cl_autowepswitch` string: the original server never reads it, the original client never sends it. Remove the client registration AND the server InfoKeyValue read (have the server-side consumer fall back to its TS behavior — check what dlls/player.cpp:5146 feeds, likely m_iAutoWepSwitch → FShouldSwitchWeapon; CTSGameRules' own switch logic is the original mechanism). |
| 14 | hud_draw | hud.cpp:316 (`m_pCvarDraw`) | hud_redraw.cpp:143 (whole-HUD gate), vgui_TeamFortressViewport.cpp:1192 | **NEEDS-GUARD** — the original's HUD-off mechanism is its own **`nohud`** cvar (registered, flags 0, matched; read at 7 sites: 0x1b024, 0x270f3(reg), 0x49f07, 0x4d3fa, 0x6389e, 0x63913, 0x64986). Remove the hud_draw registration; repoint `m_pCvarDraw` at `pfnGetCvarPointer("nohud")` is WRONG (inverted sense) — instead change the two gates to `nohud->value == 0`. VERIFY separately: the original's per-site nohud semantics (7 read sites ≠ one global redraw gate) — a follow-up RE item; the interim inverted-global gate at least kills the extra cvar without losing the feature. |
| 15 | hud_saytext | saytext.cpp:58 | saytext.cpp:102 (chat-draw gate) | **NEEDS-GUARD (1-line)** — remove registration; drop the `|| 0 == m_HUD_saytext->value` arm at line 102. (`hud_saytext_time` IS original — keep.) |
| 16 | cl_pmanstats | particleman/IParticleMan_Active.cpp:67 | :132 (debug stats overlay gate) | **SAFE-REMOVE** — debug-only. Delete registration + the stats block. (Particleman itself is SDK infra the original lacks — its 3 live call sites (entity.cpp, hud_msg.cpp, tri.cpp) are stock plumbing; removing the whole lib is a separate, bigger parity item.) |
| 17 | spec_drawnames | hud_spectator.cpp:209 | hud_spectator.cpp:660 | **NEEDS-GUARD** — see spec_* note below |
| 18 | spec_drawcone | hud_spectator.cpp:210 | hud_spectator.cpp:1691 | same |
| 19 | spec_drawstatus | hud_spectator.cpp:211 | hud_spectator.cpp (status-bar draw) | same |
| 20 | spec_autodirector | hud_spectator.cpp:212 | hud_spectator.cpp:728/742/760/846/875/1106 | same |
| 21 | spec_pip | hud_spectator.cpp:213 | hud_spectator.cpp:675/1037/1152/1155/1691/1696/1700/1769/1859–1867 + **ev_common.cpp:28 `IS_FIRSTPERSON_SPEC` macro** (live event path) | same |

### The spec_* block (17–21)

The original's spectator predates/gut-replaces the SDK-2.3 spectator: it registers **only
`cl_drawnames`** (0x22cc1 — already ported, hud_spectator.cpp:206) plus the `specmode` client
command; it has none of spec_drawnames/drawcone/drawstatus/autodirector/pip. Our
`CHudSpectator` is still the SDK-2.3 engine (overview map, inset PIP window, autodirector) with
TS re-skins (cont.84), and its code dereferences all five pointers unconditionally — deleting the
registrations alone would null-deref.

**Spec:** convert all five to **file-static, unregistered `cvar_t` instances** in
hud_spectator.cpp (e.g. `static cvar_t s_pip = {(char*)"spec_pip", (char*)"0", 0, 0.0f, nullptr};`
and point `m_pip = &s_pip;` etc.), preserving today's defaults (drawnames/drawcone/drawstatus/
autodirector = 1, pip = 0 — pip already TS-forced OFF). Delete the five `pfnRegisterVariable`
calls + the registration-failure Con_Printf check. This removes the console/config surface (the
parity contract) with zero behavior change; the eventual original-spectator reconstruction then
deletes the statics with the code they gate. Do **NOT** alias `m_drawnames` to the registered
`cl_drawnames` — the original's cl_drawnames consumer (in-eye name drawing) is NOT our overview-
map name gate; wiring them together would invent a coupling (cl_drawnames' true read sites are a
separate RE item, out of scope here per the task).

---

# Summary — suggested execution order

1. **A3 + B1–8** (one commit, `cl_dll/inputw32.cpp` + `input.cpp`): m_customaccel*→m_accel* swap,
   IN_ScaleMouse rewrite, ±89 hardcode, joysupported/m_mousethread_sleep removal.
2. **A1 regs + A2** (ts_hud.cpp/hud.cpp/ts_spectate.cpp): gl_fog* + cl_thirdperson registrations,
   Spectator-edge camera logic, ts_hud.cpp:46 comment fix.
3. **B9–12** (view.cpp/hud.cpp): roll→movevars, bobtilt branch removal, chasedist constant.
4. **B13–16**: cl_autowepswitch (client+server), hud_draw→nohud gates, hud_saytext gate,
   cl_pmanstats.
5. **B17–21**: spec_* → file-static cvar_t.
6. Re-run `scripts/coverage/coverage.py all` — client-cvar section should read
   missing **0** / extra **0**.

Open VERIFY items recorded above: FUN_10036670/FUN_10004600 (Spectator-local resets),
movevars-roll in FUN_10067890, the original nohud per-site semantics, the original spectator
chase distance. None block the fixes; each is flagged at its read site.
