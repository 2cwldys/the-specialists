# TS client↔engine API surface (the full behavioural contract)

The complete set of engine↔client API points = the behavioural surface a drop-in
client must cover. This file is the **surface scan + completeness ledger** for the
**client** (cl_dll). Recovered definitively from `reference/client.dll` by finding
the call sites of the engine function-pointer globals (MSVC stores each engine func
as its own `0x100d5bXX` global, called via `ff 15 <ptr>`):

| global | engine func | count |
|---|---|---|
| `0x100d5b60` | `pfnRegisterVariable` (cvars) | 104 |
| `0x100d5b6c` | `pfnAddCommand` (commands) | 92 |
| `0x100d5b70` | `pfnHookUserMsg` (user messages read) | 74 |
| `0x100d5c3c` | `pfnPrecacheEvent` (events) | 11 |

Re-extract: the python in the cont.52 ledger entry / session transcript. **Server**
side (ts_i386.so cvars/commands/events) is a separate scan — TODO (see bottom).

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
| `cancelselect`/`slot1..slot10`/`invnext`/`invprev` | weapon select | ⬜ stock (TS arsenal uses `tsnextweapon` server-side) |
| `firstperson`/`thirdperson`/`snapto`/`+cam*` | camera | ⬜ stock camera.cpp |

**Full command list (verbatim):** +alt1 +alt2 +attack +attack2 +back +break
+camdistance +camin +cammousemove +camout +campitchdown +campitchup +camyawleft
+camyawright +commandmenu +duck +forward +graph +jlook +jump +klook +left +lookdown
+lookup +movedown +moveleft +moveright +moveup +reload +right +score +showscores
+speed +use (and all `-` counterparts) ForceCloseCommandMenu buy cancelselect
centerview firstperson impulse invnext invprev joyadvancedupdate slot1..slot10
snapto special thirdperson togglebrowser voice_showbanned

---

## 3. Cvars the client REGISTERS (104)

**TS-specific (the gameplay/HUD cvars — Phase 3 + the HUD work):**

| cvar | default | meaning | status |
|---|---|---|---|
| `hud_r` | 150 | HUD red | ✅ registered + read (cont.52) |
| `hud_g` | 175 | HUD green | ✅ |
| `hud_b` | 255 | HUD blue | ✅ |
| `hud_color` | 150,175,255 | combined HUD colour | ✅ registered (cont.52); combined-parse 🟡 |
| `cl_realhud` | 0 | alt HUD mode (panels vs realistic) | 🟡 registered, not honoured |
| `cl_motionblur` | 1 | slow-mo motion blur | ❌ |
| `cl_gore` | 2 | gore level | ❌ |
| `cl_laser` | 0 | laser sight | ❌ |
| `cl_lasercolor` | 10 | laser colour | ❌ |
| `cl_solidcross` | 2 | crosshair style | ❌ |
| `cl_usepowerup` | 1 | auto-use powerup | ❌ |
| `cl_buykevlar` | 1 | auto-buy kevlar | ❌ |
| `cl_showequipment` | 1 | show equipment | ❌ |
| `cl_shadow` | 2 | shadows | ❌ |
| `cl_thirdperson`/`cl_thirdpersonswitch`/`cl_thirdtransparency`/`cl_camtype` | 0/0/1/0 | 3rd-person | ❌ |
| `cl_attackswitch` | 0 | auto weapon switch | ❌ |
| `cl_wavedetail`/`cl_fx`/`cl_vsmoothing`/`cl_slowfade` | | misc fx | ❌ |
| `cl_bob` | 4.0 | view bob (Phase 5 "snap") | 🟡 stock cl_bob exists |
| `hud_classautokill`/`hud_takesshots`/`hud_centerid` | 1/0/0 | misc | ❌ |
| `nohud` | 0 | hide HUD | ❌ |
| `r_paintball` | 0 | paintball mode | ❌ |
| `gl_fog*` (6) | | fog | ❌ (server `ts_fog` exists) |

The rest (~70) are **stock HL** cvars (`cl_forwardspeed`, `sensitivity`, `m_yaw`,
`cam_*`, `joy*`, `v_center*`, `scr_ofs*`, `voice_*`, `default_fov`, `bitmapfonts`,
`lookspring`, …) — ⬜ inherited, engine/stock-client owned.

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

- Cvars: 104 total; **TS-specific ~30**, of which ✅ ~25 now REGISTERED with faithful defaults
  (`cl_dll/ts_hud.cpp` TSHud_RegisterCvars — hud_r/g/b/color, cl_realhud/motionblur/gore/laser/
  lasercolor/solidcross/usepowerup/buykevlar/showequipment/shadow/thirdtransparency/camtype/
  attackswitch/thirdpersonswitch/wavedetail/fx/slowfade/vsmoothing, hud_classautokill/takesshots,
  nohud, r_paintball); the rest are stock/engine ⬜. (Registration only — most aren't yet *read*.)
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
`customweapons ""`, `banfortk 0`, `dmgreport 0`, `botarmor 0`, `deadstomach deadtable`.
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
| `hud_r`/`hud_g`/`hud_b` | ✅ | ✅ | ★ | HUD text colour (cont.52) |
| `hud_color` | ✅ | — | — | combined; not parsed |
| `cl_realhud` | ✅ | — | — | alt HUD mode not honoured |
| `cl_motionblur`/`cl_gore`/`cl_laser`/`cl_lasercolor`/`cl_solidcross`/`cl_usepowerup`/`cl_buykevlar`/`cl_showequipment`/`cl_shadow`/`cl_thirdtransparency`/`cl_camtype`/`cl_attackswitch`/`cl_thirdpersonswitch`/`cl_wavedetail`/`cl_fx`/`cl_slowfade`/`cl_vsmoothing`/`hud_classautokill`/`hud_takesshots`/`nohud`/`r_paintball` | ✅ | — | — | client display/fx — registered only |
| `mp_teammodels` | ✅ | — | — | server; team model override |
| `initial_cash`/`cashforwin`/`cashforlose` | ✅ | — | — | economy; `GiveCash` exists (cont.36), not charged |
| `damagemult` | ✅ | ✅ | ★ | scales all TS weapon damage (cont.63; 0=off→1x) |
| `respawntime`/`nopowerup`/`usecash`/`ammocount`/`bottalk`/`botlearn`/`realbullet`/`mp_theonemode`/`mp_theone`/`mp_theothers`/`lastmanstanding`/`spectacularness`/`respawnnumber`/`customweapons`/`banfortk`/`dmgreport`/`botarmor`/`deadstomach` | ✅ | — | — | server; registered only |

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
| server: `tsbuy`/`tsbuyammo`/`tsarm`/`tsbuyarmament`/`tsnextweapon`/`tsfiremode`/`dropweapon`/`usepowerup`/`tsspawnflag`/`tscutwire` | ✅ | — | ★ | console-driven gameplay (cont.23–45) |

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
