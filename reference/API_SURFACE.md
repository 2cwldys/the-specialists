# TS client↔engine API surface (the full behavioural contract)

The complete set of engine↔client API points = the behavioural surface a drop-in
client must cover. This file is the **surface scan + completeness ledger** for the
**client** (cl_dll). Recovered definitively from `reference/client.dll` by finding
the call sites of the engine function-pointer globals (MSVC stores each engine func
as its own `0x100d5bXX` global, called via `ff 15 <ptr>`):

| global | engine func | count |
|---|---|---|
| `0x100d5b60` | `pfnRegisterVariable` (cvars) | 107 |
| `0x100d5b6c` | `pfnAddCommand` (commands) | 92 |
| `0x100d5b70` | `pfnHookUserMsg` (user messages read) | 74 |
| `0x100d5c3c` | `pfnPrecacheEvent` (events) | 11 |

Re-extract: the python in the cont.52 ledger entry / session transcript. **Server**
side (ts_i386.so cvars/commands/events) is a separate scan — TODO (see bottom).

Sections 1, 2 and 4 to 7 date from 2026-07-03 and are stale (most of what they mark ❌ has since
been built); `reference/coverage/REPORT.md` (`scripts/coverage/run.py`) is the current surface diff.
Section 3 (cvars) was regenerated on 2026-10-10.

Status key: ✅ implemented/faithful · 🟡 partial/approximate · ❌ not implemented ·
⬜ stock-HL (inherited from halflife-updated cl_dll, not TS-specific).

---

## 1. User messages the client HOOKS (74) — the read side of the protocol

Send side + wire formats: see `reference/HUD_MESSAGES.md`. TS-specific messages and
our client reader status:

| msg | size | our reader | status |
|---|---|---|---|
| `RoundTime` | 2 | `CHudRoundTime` | ✅ (2-byte fixed, cont.51) |
| `TSHealth` | 1 | `CHudTSStatus` | ✅ cont.51 |
| `TSArmor` | 1 | `CHudTSStatus` | ✅ cont.51 |
| `TSCash` | 2 | `CHudTSStatus` | ✅ cont.51 |
| `WeaponInfo` | 6 | `CHudTSAmmo` | 🟡 cont.52 — reads type/clip/reserve/mode; flags byte ignored; layout approximate (NOT yet traced from the original Draw chain) |
| `TSState` | 1 | — | ❌ player-state byte (@0x498) |
| `TSSpace` | 1 | — | ❌ "free slots" (left HUD) |
| `WStatus` | 3 | — | ❌ per-slot weapon-bar status (the top weapon-select bar) |
| `ActItems` | 2 | — | ❌ active-items |
| `ClipInfo` | 1 | — | ❌ (note: server CTSGun never SENDS ClipInfo; clip rides in WeaponInfo) |
| `KFuPower` | 1 | — | ❌ kung-fu power bar |
| `PwUp` | 3 | — | ❌ powerup grant HUD |
| `PwUpTime` | 1 | — | ❌ powerup countdown bar (server sends, cont.51) |
| `TSMBlur` | 2 | — | ❌ motion-blur effect (server sends, cont.51) |
| `TSSlowMo` | 2 | — | ❌ slow-mo amount (COORD) — the visible time-dilation |
| `TSBTime` | 1 | — | ❌ bullet-time |
| `TSPAward` | 2 | — | ❌ point-award popup |
| `TSFrags` | -1 | — | ❌ scoreboard frags |
| `TSFlag` | 4 | — | ❌ CTF flag HUD |
| `TSFade` | -1 | — | ❌ screen fade |
| `TSPSys` | -1 | — | ❌ particle system |
| `TSHVoice` | 1 | — | ❌ voice |
| `TSMessage` | -1 | — | ❌ HUD text (server sends) |
| `Objective`/`ShowObj`/`Wires` | -1/2/3 | — | ❌ objective/bomb HUDs |
| `SrvSett`/`PlInfo`/`ValClass`/`ReqState`/`RandomPC`/`AllowSpec`/`BuildSt`/`Detpack`/`Feign`/`Concuss`/`SecAmmoIcon`/`SecAmmoVal`/`CustomWP`/`Spectator`/`NextStunt`/`VGUIMenu` | var | — | ❌ |
| stock: `Health`/`Battery`/`AmmoX`/`CurWeapon`/`Damage`/`DeathMsg`/`ScoreInfo`/`TeamInfo`/`TeamScore`/`TextMsg`/`SayText`/`StatusValue`/`StatusText`/`StatusIcon`/`Geiger`/`Train`/`HideWeapon`/`SetFOV`/`ShowMenu`/`MOTD`/`ServerName`/`GameMode`/`GameTitle`/`InitHUD`/`ResetHUD`/`WeaponList`/`WeapPickup`/`AmmoPickup`/`ItemPickup`/`HudText`/`Logo`/`TeamNames`/`PTakeDam`/`VGUIMenu` | | stock CHud* | ⬜ inherited |

**Full hooked list (verbatim):** ActItems AllowSpec AmmoPickup AmmoX BuildSt
ClipInfo Concuss CurWeapon CustomWP Damage DeathMsg Detpack Feign GameMode
GameTitle Geiger Health HideWeapon HudText InitHUD ItemPickup KFuPower Logo MOTD
NextStunt Objective PTakeDam PlInfo PwUp PwUpTime RandomPC ReqState ResetHUD
RoundTime SayText ScoreInfo SecAmmoIcon SecAmmoVal ServerName SetFOV ShowMenu
ShowObj Spectator SrvSett StatusIcon StatusText StatusValue TSArmor TSBTime TSCash
TSFade TSFlag TSFrags TSHVoice TSHealth TSMBlur TSMessage TSPAward TSPSys TSSlowMo
TSSpace TSState TeamInfo TeamNames TeamScore TextMsg Train VGUIMenu ValClass
WStatus WeapPickup WeaponInfo WeaponList Wires

---

## 2. Console commands the client REGISTERS (92)

Mostly stock HL input bindings (the `+/-` pairs are engine input the client
re-registers). **TS-specific commands** (the ones our cl_dll must add — currently
NONE are implemented client-side; some are handled server-side via console):

| command | meaning | status |
|---|---|---|
| `+alt1`/`-alt1` | TS alt-fire 1 (silencer/zoom/etc per weapon) | ❌ client (server has no `+alt1`) |
| `+alt2`/`-alt2` | TS alt-fire 2 | ❌ |
| `+reload`/`-reload` | reload hold | 🟡 server `+reload` drives `CTSGun::ItemPostFrame` (cont.50); client toggle ❌ |
| `+break`/`-break` | break/charge (stunt?) | ❌ |
| `special` | stunt / special move | ❌ |
| `buy` | open buy menu | 🟡 server `tsbuy`/`tsbuyarmament` (cont.36); client menu ❌ |
| `togglebrowser` | server browser | ❌ |
| `ForceCloseCommandMenu` | command menu | ❌ |
| `+commandmenu`/`-commandmenu` | command/class menu | ❌ |
| `invnext`/`invprev` | weapon select | ★ wired (cont.67): client cursor walks the owned arsenal → emits `weapon_<N>` (see below). `cancelselect`/`slot1..slot10` ⬜ stock |
| `weapon_<N>` (numeric) / `lastinv` | weapon-select commit / last-weapon toggle | ★ wired (cont.67, RE `FUN_1004f500`/`SelectWeapon`@0x80fb8): server `TSSelectWeaponType`/`TSLastInv` → `SelectSlot` holster→deploy. `reference/WEAPON_SELECT.md` |
| `firstperson`/`thirdperson`/`snapto`/`+cam*` | camera | ⬜ stock camera.cpp |

**Full command list (verbatim):** +alt1 +alt2 +attack +attack2 +back +break
+camdistance +camin +cammousemove +camout +campitchdown +campitchup +camyawleft
+camyawright +commandmenu +duck +forward +graph +jlook +jump +klook +left +lookdown
+lookup +movedown +moveleft +moveright +moveup +reload +right +score +showscores
+speed +use (and all `-` counterparts) ForceCloseCommandMenu buy cancelselect
centerview firstperson impulse invnext invprev joyadvancedupdate slot1..slot10
snapto special thirdperson togglebrowser voice_showbanned

---

## 3. Cvars the client REGISTERS (107)

Regenerated 2026-10-10 from `reference/client.dll`: every `pfnRegisterVariable` call (name, default,
flags, where the `cvar_t*` is kept), every read of the kept pointer and every by-name
`pfnGetCvarFloat/String/Pointer` call, each read decompiled (`scripts/ghidra/dc.sh client`). "reTS
before" is the client before mod-ts's client-cvars round; "verdict" is the state after it. Engine cvars
the client only reads (`r_decals`, `cl_lw`, ...) are left out. `default_fov` is registered as `"%i"` of
80 with flags 0 and its value then set to 80.0. user.scr's `mp_decals` is an engine cvar.

| cvar | default | flags | client.dll reads | what it does in client.dll | reTS before | verdict |
|---|---|---|---|---|---|---|
| voice_modenable | "1" | ARCHIVE | 2 by name | stock SDK use | same | same |
| voice_clientdebug | "0" | 0 | 11 by name | stock SDK use | same | same |
| hud_drawhistory_time | "5" | 0 | 2 by name | stock SDK use (pickup history) | same | same |
| hud_fastswitch | "0" | ARCHIVE | 2 by name | slot keys: 0 opens the select at the column and FIRE commits, else the slot switches at once (0x1004f220) | stock gWR path only; the TS bar ignored it | fixed (slot handler port) |
| hud_deathnotice_time | "6" | 0 | 1 by name | stock SDK use | flags ARCHIVE | fixed (flags 0) |
| hud_classautokill | "1" | ARCHIVE|USERINFO | 2 by name | TFC class menu leftover (0x1005a460, 0x1005ab10) | same | same |
| hud_takesshots | "0" | ARCHIVE | 1 by name | screenshot at intermission (CHud::Redraw) | same | same |
| r_paintball | "0" | ARCHIVE | 3 | every bullet decal becomes {splat0..17} (0x10001000) | registered, never read | fixed |
| zoom_sensitivity_ratio | "1.2" | 0 | 2 by name | zoomed mouse scale (0x10049760, SetFOV) | flags ARCHIVE | fixed (flags 0) |
| gl_fog | "0" | ARCHIVE | 0 + 2 by name | 0 no fog; 1 the map's ts_fog; 2 single player user fog (0x10056c20) | registered, never read | not done: needs the ts_fog entity-lump parser and a fog pass; default 0 is no fog on both |
| gl_fog_start | "0" | 0 | 1 by name | gl_fog 2 fog start | registered, never read | not done (gl_fog) |
| gl_fog_end | "2048" | 0 | 1 by name | gl_fog 2 fog end | registered, never read | not done (gl_fog) |
| gl_fog_red | ".5" | 0 | 1 by name | gl_fog 2 colour | registered, never read | not done (gl_fog) |
| gl_fog_green | ".5" | 0 | 1 by name | gl_fog 2 colour | registered, never read | not done (gl_fog) |
| gl_fog_blue | ".5" | 0 | 1 by name | gl_fog 2 colour | registered, never read | not done (gl_fog) |
| default_fov | "80" | 0 | CHud Think 0x10049760 | the unzoomed FOV, held to 70..110, 80 below 0.001; value forced to 80.0 at registration | flags ARCHIVE; read the same way | fixed (flags 0); see the default_fov section |
| hud_capturemouse | "1" | ARCHIVE | viewport mouse | stock SDK use | same | same |
| cl_drawnames | "1" | ARCHIVE | 1 (0x1002318f) | overview map player names (0x1002318f, the stock spec_drawnames test) | flags 0; never read (an unregistered spec_drawnames gated the names) | fixed (ARCHIVE, gates the names) |
| cam_command | "0" | 0 | 1 | stock SDK use | same | same |
| cam_snapto | "0" | 0 | 2 | stock SDK use | same | same |
| cam_idealyaw | "0" | 0 | 4 | stock SDK use | same | same |
| cam_idealpitch | "0" | 0 | 4 | stock SDK use | same | same |
| cam_idealdist | "100" | 0 | 2 | stock camera distance (TS camera ignores it) | default "64" | fixed (default "100") |
| cam_contain | "0" | 0 | 0 | never read | read by stock CAM_Think (no effect at 0) | kept, reported |
| c_maxpitch | "90.0" | 0 | 2 | stock SDK use | same | same |
| c_minpitch | "0.0" | 0 | 2 | stock SDK use | same | same |
| c_maxyaw | "135.0" | 0 | 2 | stock SDK use | same | same |
| c_minyaw | "-135.0" | 0 | 2 | stock SDK use | same | same |
| c_maxdistance | "200.0" | 0 | 2 | stock SDK use | same | same |
| c_mindistance | "30.0" | 0 | 2 | stock SDK use | same | same |
| lookstrafe | "0" | ARCHIVE | 1 | stock SDK use | same | same |
| lookspring | "0" | ARCHIVE | 2 | stock SDK use | same | same |
| cl_anglespeedkey | "0.67" | 0 | 1 | stock SDK use | same | same |
| cl_yawspeed | "210" | 0 | 2 | stock SDK use | same | same |
| cl_pitchspeed | "225" | 0 | 4 | stock SDK use | same | same |
| cl_upspeed | "320" | 0 | 2 | stock SDK use | same | same |
| cl_forwardspeed | "400" | ARCHIVE | 3 | stock SDK use | same | same |
| cl_backspeed | "400" | ARCHIVE | 1 | stock SDK use | same | same |
| cl_sidespeed | "400" | 0 | 6 | stock SDK use | same | same |
| cl_movespeedkey | "0.3" | 0 | 4 | stock SDK use | same | same |
| cl_vsmoothing | "0.05" | ARCHIVE | 6 | stair/view smoothing, also gated by stunt camera flags 0x101914ec | stock smoothing (no stunt camera in reTS) | same while no stunt camera runs |
| m_pitch | "0.022" | ARCHIVE | 2 | stock SDK use | same | same |
| m_yaw | "0.022" | ARCHIVE | 1 | stock SDK use | same | same |
| m_forward | "1" | ARCHIVE | 2 | stock SDK use | same | same |
| m_side | "0.8" | ARCHIVE | 0 | never read (no +strafe mouse path) | read by stock IN_MouseMove for +strafe | kept, reported |
| cl_fx | "1" | ARCHIVE | 5 | muzzle smoke gate, motion blur frames cap, particle config (0x10057d50), extra FX pass at 1 | smoke gate and blur cap read | partial: particle config and the FX root are not reconstructed |
| cam_back | "90" | ARCHIVE | 1 | cl_camtype 3 back distance | flags 0 | fixed (ARCHIVE) |
| cam_right | "0" | ARCHIVE | 1 | cl_camtype 3 side offset | flags 0 | fixed (ARCHIVE) |
| cam_up | "12" | ARCHIVE | 1 | cl_camtype 3 height | flags 0 | fixed (ARCHIVE) |
| cl_camtype | "0" | ARCHIVE | 1 | third person camera preset 0..3 (V_CalcNormalRefdef) | flags 0 (in_camera.cpp registered first) | fixed (ARCHIVE) |
| hud_r | "150" | ARCHIVE | 28 | HUD colour | same | same |
| hud_g | "175" | ARCHIVE | 28 | HUD colour | same | same |
| hud_b | "255" | ARCHIVE | 28 | HUD colour | same | same |
| hud_color | "150,175,255" | ARCHIVE | 0 | never read | never read | same |
| cl_lasercolor | "10" | ARCHIVE | 1 | sends lasclr_<n> on change (0x1004ce10) | same | same |
| cl_buykevlar | "1" | ARCHIVE | 1 | an emptied buy config starts with kevlar (0x10038800); never written | never seeded kevlar; written on every Apply | fixed |
| nohud | "0" | 0 | 6 | hides the death notices, the status draw's parts, the weapon bar after the laser dot, the spectator panel | flags ARCHIVE; hid every HUD element | fixed (flags 0, per element) |
| cl_realhud | "0" | ARCHIVE | 3 | not 0 hides the status panel, the weapon panel (not kung fu's) and the crosshair draw | registered, never read | fixed |
| cl_shadow | "2" | ARCHIVE | 2 | ombra.spr blob shadows: 0 off, 1 also your own in first person, else others | same | same |
| cl_wavedetail | "0" | ARCHIVE | 1 | bullet trail waves in the client bullet simulation (0x10035010) | registered, never read | not done: reTS has no client bullet simulation |
| cl_thirdpersonswitch | "0" | ARCHIVE | 2 | 1: third person with kung fu, back on a weapon (WeaponInfo) | same | same |
| cl_usepowerup | "1" | ARCHIVE | 1 | double +use within 0.3 s sends usepowerup | same (no test of 0x100d43e0) | same, reported |
| cl_laser | "0" | ARCHIVE | 5 | laser beam direction and dot style | read (dot settle flag VERIFY) | same, reported |
| cl_solidcross | "2" | ARCHIVE | 1 | crosshair ring style 0/1/2 | same | same |
| cl_thirdperson | "0" | ARCHIVE | 2 | camera on spawn: 1 third, 0 first | same | same |
| cl_attackswitch | "0" | ARCHIVE | 4 | swaps +attack2 and +alt2 (coldcock) except on kung fu, grenades, knives, katana, flag | registered, never read | fixed |
| cl_thirdtransparency | "1" | ARCHIVE | 1 | third person self fade when aiming at a player | same | same |
| cl_motionblur | "1" | ARCHIVE | 3 | slow-motion ghost trails 0/1/2 | same | same |
| cl_gore | "2" | ARCHIVE | 1 | blood particle count table | same (10..11 truncates) | same |
| cl_showequipment | "1" | ARCHIVE | 1 | passive weapon bar alpha 0/0.25/0.5/1 | same | same |
| m_filter | "0" | ARCHIVE | 1 | stock SDK use | same | same |
| sensitivity | "3" | ARCHIVE | 3 | stock SDK use | flags ARCHIVE|FILTERSTUFFTEXT | fixed (ARCHIVE) |
| m_accel | "0" | ARCHIVE | 1 | stock SDK use | same | same |
| m_accelexp | "1.2" | ARCHIVE | 1 | stock SDK use | same | same |
| m_accelmax | "0" | ARCHIVE | 1 | stock SDK use | same | same |
| joystick | "0" | ARCHIVE | 2 | stock SDK use | same | same |
| joyname | "joystick" | 0 | 1 | stock SDK use | same | same |
| joyadvanced | "0" | 0 | 2 | stock SDK use | same | same |
| joyadvaxisx | "0" | 0 | 1 | stock SDK use | same | same |
| joyadvaxisy | "0" | 0 | 1 | stock SDK use | same | same |
| joyadvaxisz | "0" | 0 | 1 | stock SDK use | same | same |
| joyadvaxisr | "0" | 0 | 1 | stock SDK use | same | same |
| joyadvaxisu | "0" | 0 | 1 | stock SDK use | same | same |
| joyadvaxisv | "0" | 0 | 1 | stock SDK use | same | same |
| joyforwardthreshold | "0.15" | 0 | 1 | stock SDK use | same | same |
| joysidethreshold | "0.15" | 0 | 2 | stock SDK use | same | same |
| joypitchthreshold | "0.15" | 0 | 2 | stock SDK use | same | same |
| joyyawthreshold | "0.15" | 0 | 1 | stock SDK use | same | same |
| joyforwardsensitivity | "-1.0" | 0 | 1 | stock SDK use | same | same |
| joysidesensitivity | "-1.0" | 0 | 2 | stock SDK use | same | same |
| joypitchsensitivity | "1.0" | 0 | 4 | stock SDK use | same | same |
| joyyawsensitivity | "-1.0" | 0 | 2 | stock SDK use | same | same |
| joywwhack1 | "0.0" | 0 | 1 | stock SDK use | same | same |
| joywwhack2 | "0.0" | 0 | 1 | stock SDK use | same | same |
| hud_saytext_time | "5" | 0 | 1 by name | stock SDK use | same | same |
| hud_centerid | "0" | ARCHIVE | 2 by name | stock SDK use | same | same |
| bitmapfonts | "1" | 0 | 1 | stock SDK use | same | same |
| scr_ofsx | "0" | 0 | 0 | never read | read by stock view offset (no effect at 0) | kept, reported |
| scr_ofsy | "0" | 0 | 0 | never read | read (no effect at 0) | kept, reported |
| scr_ofsz | "0" | 0 | 0 | never read | read (no effect at 0) | kept, reported |
| v_centermove | "0.15" | 0 | 1 | stock SDK use | same | same |
| v_centerspeed | "500" | 0 | 2 | stock SDK use | same | same |
| cl_bobcycle | "0.8" | 0 | 0 | never read | read by the stock bob | fixed (no longer read) |
| cl_bob | "4.0" | ARCHIVE | 1 | weapon bob level 0..3 and 4+ (0x10066e70) | default "0.01"; stock bob computed and unused | fixed (default "4.0", the bob ported) |
| cl_bobup | "0.5" | 0 | 0 | never read | read by the stock bob | fixed (no longer read) |
| cl_waterdist | "4" | 0 | 1 | stock SDK use | same | same |
| cl_slowfade | "1" | 0 | 1 | slow-motion screen tint gate (V_CalcRefdef) | flags ARCHIVE | fixed (flags 0) |

Not done, with the reason: `gl_fog*` (the `ts_fog` entity-lump parser and the fog pass, client.dll
0x1003a5d0/0x10056c20, are not reconstructed; default 0 is no fog on both clients), `cl_wavedetail`
(client.dll's client-side bullet simulation 0x10035010 has no reTS counterpart), and `cl_fx`'s particle
configuration and FX-root reads (0x10057d50, 0x10056dd0).

---

## 4. Events the client PRECACHES (11) — events/*.sc

All fired by the server's `CTSGun` (PLAYBACK_EVENT). Client-side callbacks
reconstruct predicted fire/anim/sound. **NONE implemented client-side yet** (Phase 5
— we currently drive anims server-side via `SendWeaponAnim`):

closecombat, coldcock, draw, empty, fire, firelinked, grenade, katana, knife,
pump, reload — all ❌ client EV_ callbacks (server precache/playback ✅).

---

## 5. HL client engine exports (HUD_*) — the C interface

The engine calls these every tick / on events. Inherited from halflife-updated and
present (the client loads + runs), but TS-specific behaviour inside several is
missing (e.g. `HUD_PostRunCmd` weapon prediction, `CL_CreateMove` for TS movement,
`HUD_UpdateClientData`). Per-tick chain: `HUD_Frame` → `HUD_Redraw` (all CHud Draw)
→ `HUD_UpdateClientData`; input: `CL_CreateMove`; prediction: `HUD_PostRunCmd`.
Status: ⬜ present, TS-specific internals 🟡/❌ (tracked under Phases 1/2/5).

---

## 6. Coverage summary (client)

- Cvars: 107, all registered with client.dll's defaults and flags; per-cvar state in section 3.
- Commands: 92 total; **TS-specific ~12 now REGISTERED** (`cl_dll/ts_commands.cpp`): `+alt1`/`+alt2`/
  `+break`/`special`/`buy` forward to the server (accepted in `dlls/client.cpp` ClientCommand, gameplay
  follow-on); `+commandmenu`/`togglebrowser`/`ForceCloseCommandMenu` are client UI stubs. No more
  "Unknown command" spam; binds no longer error. Behaviour (alt-fire/coldcock/stunts/buy menu) is TODO.
- User messages: 74 hooked; **TS-specific ~30**, ✅5 readers, 🟡1, ❌~24.
- Events: 11; ❌ all client callbacks (server side ✅).

## 6b. Server cvars (ts_i386.so) — SCANNED + registered (cont.54)

Extracted from the `.so` cvar_t structs (R_386_RELATIVE relocs → name/default string
pairs). TS-specific game cvars our server lacked, now registered in `dlls/ts_cvars.cpp`
(`TSRegisterCvars` from GameDLLInit) with the exact defaults:
`mp_teammodels ""`, `initial_cash 1200`, `cashforwin 10000`, `cashforlose 1200`,
`roundtime 150` (WIRED → the round timer, was hardcoded 120), `respawntime 3`,
`damagemult 0.0`, `slowmatch 1.0`, `nopowerup 0.0`, `botlearn 0.0`, `realbullet 0.0`,
`usecash 0.0`, `ammocount 0`, `bottalk 1`, `mp_theonemode 0`, `mp_theone gordon`,
`mp_theothers seal`, `lastmanstanding 0`, `spectacularness 1`, `respawnnumber 1`,
`customweapons ""`, `banfortk 0`, `dmgreport 0`, `botarmor 0`. (~~`deadstomach deadtable`~~ was an
extraction FALSE POSITIVE — a CDeadHEV/CDeadHGrunt pose-string pair in .data, not a cvar_t.)
(The standard mp_* — mp_teamplay/friendlyfire/… — the base SDK registers under bare
names; the original uses the mp_ prefix. NOTE: a faithful pass could alias those.)
Registration stops "Unknown command mp_teammodels" etc; most still need behaviour wiring
(only `roundtime` is read so far).

## 6c. Implementation status — TS cvars & commands (R=registered, ✓read, ★gameplay-wired)

Three levels: **R** = exists (no "Unknown command"); **read** = a consumer reads its value; **wired** =
it changes gameplay. Most are R-only so far.

### TS-specific cvars
| cvar | R | read | wired | notes |
|---|:-:|:-:|:-:|---|
| `roundtime` | ✅ | ✅ | ★ | drives StartRoundTimer (cont.54) |
| `slowmatch` | ✅ | ✅ | ★ | global slow-motion match (cont.59) |
| client cvars | | | | see section 3 |
| `mp_teammodels` | ✅ | — | — | server; team model override |
| `initial_cash` | ✅ | ✅ | ★ | starting cash on spawn → cash HUD (cont.63c, verified "$5000") |
| `cashforwin`/`cashforlose` | ✅ | — | — | round-end economy; `GiveCash`/`TeamWin` exist, not awarded yet |
| `damagemult` | ✅ | ✅ | ★ | scales all TS weapon damage (cont.63; 0=off→1x) |
| `respawntime`/`nopowerup`/`usecash`/`ammocount`/`bottalk`/`botlearn`/`realbullet`/`mp_theonemode`/`mp_theone`/`mp_theothers`/`lastmanstanding`/`spectacularness`/`respawnnumber`/`customweapons`/`banfortk`/`dmgreport`/`botarmor` | ✅ | — | — | server; registered only |

### TS-specific commands
| command | R | forwarded | gameplay | notes |
|---|:-:|:-:|:-:|---|
| `+alt2`/`-alt2` | ✅ | ✅ | ★ | coldcock pistol-whip (cont.57) |
| `+alt1`/`-alt1` | (xash built-in) | ✅ | ❌ | silencer/scope — UNMAPPED (see VERIFICATION.md §B) |
| `special` | (xash built-in) | ✅ | ❌ | stunt — needs CTSStunt |
| `+break`/`-break` | (xash built-in) | ✅ | ❌ | — |
| `buy` | ✅ | ✅ | ❌ | buy menu (cl_dll VGUI); server `tsbuy*` exist (cont.36) |
| `+commandmenu`/`-commandmenu`/`ForceCloseCommandMenu` | (xash built-in) | — | ❌ | class/command menu (cl_dll) |
| `togglebrowser` | ✅ | — | ❌ | server browser (cl_dll) — stub |
| server: `weapon_<N>`/`lastinv` (faithful, cont.67) + `tsbuy`/`tsbuyammo`/`tsarm`/`tsbuyarmament`/`tsfiremode`/`dropweapon`/`usepowerup`/`tsspawnflag`/`tscutwire` (stand-ins) | ✅ | — | ★ | console-driven gameplay (cont.23–45,67). NB stand-in names: real ones are `firemode`/`drop`/`usepowerup`(real)/buy=`tkw`/`tki`/`tka` — see `reference/WEAPON_SELECT.md` |

## 7. TODO — to complete the surface scan
- **Server commands** (`pfnAddServerCommand`) + the events the server PLAYBACK_EVENTs —
  same reloc/GOT method; not yet enumerated. (Server *console* commands like tsbuy are
  already in `dlls/client.cpp` ClientCommand.)
- The standard mp_*-prefix cvar aliases (vs the base SDK's bare names).
- **HUD draw chain**: trace each TS `CHud*::Draw` in client.dll from assembly to
  recover EXACT panel geometry, FillRGBA colours/alpha, font, and text positions
  (current panels are approximate — flagged by the user). Start from the HookUserMsg
  handler for `WeaponInfo`/`WStatus`/`TSHealth` → the class vtable → its Draw.
- Cross-check each ❌ message against whether the SERVER sends it (some need both
  halves built).
