# cvar → game-settings mapping audit (settings.scr / user.scr)

READ-ONLY audit. Source of the mapping: the deployed TS game dir listen/dedicated-server
settings UI scripts:
- `ts/settings.scr` → `DESCRIPTION SERVER_OPTIONS` (20 entries — the server-side gameplay knobs).
- `ts/user.scr` → `DESCRIPTION INFO_OPTIONS` (19 entries — client display / userinfo knobs).

(identical copies live under `mods/ts/*.scr` and `recovered/*/ts/*.scr`.)

Each `.scr` entry is `"cvar" { "Label" { type[info] } { default } }`. The listen-server GUI
writes the chosen value to the server/user config; the engine then either sets the cvar or
**executes it as a console command** (this distinction is the crux of two findings below —
`globalslow` and `weaponrestriction` are COMMANDS in the original, not cvars).

Legend for scope: **TS-DLL** = registered by the TS game DLL (`dlls/`); **TS-cl** = registered by
the TS client (`cl_dll/`); **engine** = HLDS/HL builtin (not our DLL's job to register).

---

## Table A — SERVER_OPTIONS (settings.scr) — server gameplay

| cvar | label | scr default | cvar_t default (ours) | scope | registered by us? | read by us (file:line) | original read/handler | effect | STATUS |
|---|---|---|---|---|---|---|---|---|---|
| `mp_fraglimit` | Frag Limit | 0 | 0 (`game.cpp:32` var `fraglimit`) | engine/mp | YES `game.cpp:569` | engine gamerules | frag cap | ok |
| `mp_timelimit` | Time Limit (Min.) | 30 | 0 (`game.cpp:33` var `timelimit`) | engine/mp | YES `game.cpp:570` | engine gamerules | map time cap | ok |
| `initial_cash` | **Starting Credits** | 3000 | 1200 | TS-DLL | YES `ts_cvars.cpp:54` | `TS_gamerulesclass.cpp:443` (team seed), `:1059` (`GiveCash` on spawn/join) | starting buy budget | **ok** (see note 1) |
| `roundtime` | Round Time (sec.) | 180 | 150 | TS-DLL | YES `ts_cvars.cpp:57` | `TS_gamerulesclass.cpp:36` (`RoundTimeLeft`) | round length | ok |
| `respawntime` | Respawn Delay (sec.) | 3 | 3 | TS-DLL | YES `ts_cvars.cpp:58` | `player.cpp:920`, `player.cpp:1446` | respawn delay | ok |
| `globalslow` | Permanent Slow-motion? | 0 | — | **TS-DLL COMMAND** | **NO** | **NO** | `cmd_globalslow` @0xb5580 (parses percent) → `SetGlobalSlowMotion` | permanent global slow-mo | **MISSING-COMMAND** (see fix 1) |
| `mp_teamplay` | Teamplay in DM maps? | 0 | 0 (`game.cpp:31`) | engine/mp | YES `game.cpp:568` | `gamerules.cpp:399`, `client.cpp:1121` | team mode select | ok |
| `mp_teamlist` | Teams list | team1;… | "hitman;gordon" | engine/mp | YES `game.cpp:582` | `teamplay_gamerules.cpp:46` | team names | ok |
| `mp_teammodels` | Teams model list | seal;merc\|… | "" | TS-DLL (var `teammodels`) | YES `ts_cvars.cpp:53` | **NO** | orig `g_teamModels` — per-team player models | **MISSING-READ** (see fix 3) |
| `usecash` | Cash based DeathMatch? | 0 | 0 (var `dmcashbased`) | TS-DLL | YES `ts_cvars.cpp:65` | `TS_gamerulesclass.cpp:728`, `:1103` (→ client id 1) | toggles cash economy in DM | ok |
| `slowmatch` | SlowMatch? | 1 | 1 | TS-DLL | YES `ts_cvars.cpp:61` | `TS_gamerulesclass.cpp:658` (`GlobalSlowMotion2`) | whole-match slow-mo scaling | ok |
| `mp_theonemode` | The One Mode? | 0 | 0 | TS-DLL | YES `ts_cvars.cpp:68` | `multiplay_gamerules.cpp:287`, `TS_gamerules.cpp:225` | "The One" game mode | ok |
| `lastmanstanding` | Last Man Standing? | 0 | 0 | TS-DLL | YES `ts_cvars.cpp:71` | `multiplay_gamerules.cpp:289`, `TS_gamerules.cpp:230` | LMS game mode | ok |
| `superjump` | Superjump? | 0 | — | (none) | **NO** | **NO** | **no `superjump` cvar exists in `ts_i386.so`** | ok-parity (see note 2) // VERIFY |
| `spectacularness` | Spectacularness % | 50 | 1 | TS-DLL | YES `ts_cvars.cpp:72` | `TS_gamerules.cpp:355` (`DoSpectacularNess`) | slow-mo-on-kill probability | ok |
| `ammocount` | Ammo loadouts | 0 | 0 (var `cv_ammocount`) | TS-DLL | YES `ts_cvars.cpp:66` | `TS_gamerulesclass.cpp:737`, `:1107` → client `ts_status.cpp:172` `Set_ammo_count` | max-carry ammo profile (Normal/Realistic/Survival/Desperado) | ok |
| `weaponrestriction` | **Weapon restrictions** | 0 | — | **TS-DLL COMMAND** | YES `game.cpp:597` (`AddServerCommand`) | `game.cpp:499` `cmd_weaponrestriction` → `WeaponRestrictionByID` → `rules->m_iWeaponRestriction`; gates buy `ts_buy.cpp:153` `IsRestrictedWeapon` | which weapon categories are buyable/spawn (knife/pistol/smg/rifle/shotgun bitmask) | **ok** (see note 3) |
| `realbullet` | **Bullet physics detail** | 0 | 0 | TS-DLL | YES `ts_cvars.cpp:64` | `TS_gamerulesclass.cpp:719`,`:1096` → `m_iSrvRealBullet` (rules+0x578); gate `TS_combat.cpp:57` | non-zero ⇒ bullets are dodgeable flying projectiles (CBulletManager) instead of hitscan | **ok server-side** (see note 4) |
| `mp_friendlyfire` | Friendly Fire | 0 | 0 (`game.cpp:34` var `friendlyfire`) | TS-DLL registers | YES `game.cpp:575` | `ts_player.cpp:914`,`:991`, `teamplay_gamerules.cpp:406` | teammate damage gate | ok |
| `sv_sendvelocity` | Low lag server physics and decals | 1 | — | **engine** | NO (correct) | NO | **no string in `ts_i386.so`** — HLDS/engine cvar | ok-parity (engine) // VERIFY |

## Table B — INFO_OPTIONS (user.scr) — client display / userinfo

| cvar | label | scr default | scope | registered by us? | read by us (file:line) | STATUS |
|---|---|---|---|---|---|---|
| `mp_decals` | Decals Limit | 300 | engine | NO (engine) | engine | ok |
| `hud_r` | Hud color red | 125 | TS-cl | YES `ts_hud.cpp:38` | `ts_hud.cpp:83` | ok |
| `hud_g` | Hud color green | 150 | TS-cl | YES `ts_hud.cpp:39` | `ts_hud.cpp` (GetHudColor) | ok |
| `hud_b` | Hud color blue | 165 | TS-cl | YES `ts_hud.cpp:40` | `ts_hud.cpp` (GetHudColor) | ok |
| `hud_fastswitch` | Fast weapons switch | 0 | client | YES `ammo.cpp:293` | `ammo.cpp:439` | ok |
| `cl_showequipment` | Show equipment? | 1 | TS-cl | YES `ts_hud.cpp:56` | **NO** | **MISSING-READ** (display) |
| `cl_usepowerup` | Double-tap use powerup | 1 | TS-cl | YES `ts_hud.cpp:54` | `input.cpp:749` | ok |
| `cl_buykevlar` | Buy kevlar by default? | 1 | TS-cl | YES `ts_hud.cpp:55` | set `ts_buymenu.cpp:550`; read by buy default | ok (partial) // VERIFY |
| `cl_bob` | Weapon/View Bobbing | 3 | engine/HL | YES `view.cpp:2039` (float 0.01) | view bob | ok — scr treats as LIST 0-4, HL cvar is a float; semantics differ // VERIFY |
| `cl_camtype` | Thirdperson Camera Type | 0 | TS-cl | YES `in_camera.cpp:514` | `view.cpp:857` | ok |
| `cl_thirdpersonswitch` | AutoSwitch to thirdperson | 0 | TS-cl | YES `ts_hud.cpp:61` | **NO** | **MISSING-READ** (display) |
| `cl_thirdtransparency` | Transparent 3rd person when aiming | 0 | TS-cl | YES `ts_hud.cpp:58` | **NO** | **MISSING-READ** (display) // VERIFY vs thirdperson_transparency.md |
| `cl_fx` | Special effects detail | 1 | TS-cl | YES `ts_hud.cpp:63` | **NO** | **MISSING-READ** (display) |
| `cl_motionblur` | Motion blur? | 1 | TS-cl | YES `ts_hud.cpp:49` | **NO** | **MISSING-READ** (display) |
| `cl_gore` | Blood Amount | 2 | TS-cl | YES `ts_hud.cpp:50` | **NO** | **MISSING-READ** (display) |
| `cl_shadow` | Draw shadows? | 2 | TS-cl | YES `ts_hud.cpp:57` | `ts_attachments.cpp:308` | ok |
| `cl_wavedetail` | Bullet trails? | 0 | TS-cl | YES `ts_hud.cpp:62` | **NO** | **MISSING-READ** (display) |
| `cl_solidcross` | CrossHair style | 2 | TS-cl | YES `ts_hud.cpp:53` | **NO** | **MISSING-READ** (display) |
| `cl_laser` | Lasersight style | 0 | TS-cl | YES `ts_hud.cpp:51` | **NO** (we read `cl_lasercolor`, a different cvar, in `ts_attachments.cpp:378/416/513`) | **MISSING-READ** (display) // VERIFY |

---

## Notes on the priority items

**Note 1 — Starting Credits (`initial_cash`): HONORED.** Registered `ts_cvars.cpp:54`, read on
spawn/join via `GiveCash((int)CVAR_GET_FLOAT("initial_cash"))` (`TS_gamerulesclass.cpp:1059`) and
as the per-team seed (`:443`). One cosmetic mismatch only: our registered cvar_t default is
`1200` while the `.scr` UI default is `3000`. The cvar_t default is the parity-relevant value
(byte-faithful to `ts_i386.so` @0x108e48); the `.scr` "3000" is just the listen-server GUI's
last-stored suggestion and does not affect a dedicated server that sets the cvar. No action
needed unless a fresh listen-server must match the GUI default.

**Note 2 — Superjump: NOT a real cvar.** `strings ts_i386.so` has no bare `superjump` cvar — only
`superjump_land` (animation) and `player/superjump-land.wav`. Superjump in TS is a *map-global
mode* (rules+0x5885 / `ts_mapglobals` spawnflag 0x40) and the `item_superjump`/`CTSSuperJump`
pickup, both already modelled (`ts_player.cpp:701`, `hldmcompat.cpp:250`). The `.scr` "superjump"
BOOL is a legacy/no-op UI toggle the original DLL never reads. Our not-registering it = parity.

**Note 3 — Weapon restrictions: HONORED (as a COMMAND).** The original exposes `weaponrestriction`
as a server *command* (`cmd_weaponrestriction` @0xb58a0), not a cvar. The engine executes the
`.scr`-chosen value as `weaponrestriction <id>`; the handler maps id→mask via `WeaponRestrictionByID`
(1→knives-only 0x0? see `TS_gamerules.cpp:243`: 1→0x0, 2→0x1, 3→0x3, 4→0x13, else all 0x1f) ANDed
with `m_iAllowedCategories`, storing `rules->m_iWeaponRestriction`. Buying is gated by
`IsRestrictedWeapon(type, m_iWeaponRestriction)` (`ts_buy.cpp:153`) and the mask is synced to the
client (SrvSett id 3). We register and implement the identical command (`game.cpp:597`). Parity holds.

**Note 4 — Bullet physics detail (`realbullet`): server gate HONORED; client detail levels NOT
consumed.** The server gate in the original `ShootBulletsPlayer` @0x5f6f0 is a **boolean**:
`*(int*)(rules+0x578) != 0 || GiveSlowMul() < 0.99` → route pellets through the tracked-projectile
path. Our `TS_combat.cpp:57` is byte-identical (`m_iSrvRealBullet != 0 || GiveSlowMul() < 0.99f`).
So at the user's setting **Low (0) the server does plain hitscan — correct and honored**; Medium(1)
and High(2) both flip to projectile bullets, exactly as the original (the server does not
distinguish 1 from 2). The Low/Medium/High *granularity* is therefore a **client-display**
distinction (tracer/decal/whizz density): the value is sent to the client and stored in
`g_iTSRealBullet` (`ts_status.cpp:170`) but our client **never reads `g_iTSRealBullet` again** — it
is write-only. If the original client.dll scaled visual bullet-effect density by this value, that is
an unimplemented client-display nuance. It does **not** change server gameplay, and at Low there is
nothing to render anyway, so the user's Low setting is fully respected. // VERIFY against
`reference/client.dll` whether it consumes the realbullet detail level for FX density.

---

## What we're missing — ranked, with concrete fixes

### 1. `globalslow` command — server ignores "Permanent Slow-motion" (real gameplay gap)
- **Symptom:** a server that sets `globalslow <n>` (from the GUI or server.cfg) gets "Unknown
  command"; the permanent slow-motion never engages. The machinery already exists
  (`CTSGameRules::SetGlobalSlowMotion` / `GlobalSlowMotion2`, `TS_gamerulesclass.cpp:754/777`) — only
  the command binding is missing.
- **Original behavior** (`cmd_globalslow` @0xb5580, disassembled):
  - `argc < 2` → print current: "Globalslow is at %.0f%%." (scale*100) or "Globalslow is off."
  - else `scale = atof(argv1) * 0.01`; clamp `scale = min(scale, 2.0)` then `max(scale, 0.0)`;
    if `0.999 < scale < 1.001` → `scale = 0` (100% means OFF); if `0 < scale < 0.01` → `scale = 0.01`.
  - if `scale == 0`: if it differs from current (rules+0x564) set rules+0x560 = 0 (stop);
    else `SetGlobalSlowMotion(scale, 0.0)` when it differs from current.
  - print "Globalslow is at %.0f%%." (scale*100) or "Globalslow is off."
- **Fix:** add `cmd_globalslow()` in `dlls/game.cpp` next to `cmd_weaponrestriction`, and register it
  in the same block: `g_engfuncs.pfnAddServerCommand("globalslow", cmd_globalslow);` (near
  `game.cpp:597`). Reuse `TSGameRules()->SetGlobalSlowMotion(scale, 0.0f)` for the non-zero path and
  the m_flGlobalSlowScale / stop-field for the zero path (rules+0x560/0x564 map to the existing
  `m_flGlobalSlowScale` / next-slow members in `ts_gamerules.h:196`). Note the `.scr` range is 0–200
  (percent → 0.0–2.0 scale); the header comment at `ts_gamerules.h:196` already anticipates this.

### 2. Client display cvars registered but never read (client-visual parity, lower priority)
`cl_showequipment`, `cl_thirdpersonswitch`, `cl_thirdtransparency`, `cl_fx`, `cl_motionblur`,
`cl_gore`, `cl_wavedetail`, `cl_solidcross`, `cl_laser` are all `CVAR_CREATE`'d in `ts_hud.cpp` but
have **zero read sites** in `cl_dll/`. They set no "Unknown command" (registered), but the features
they toggle are not wired:
- `cl_gore` (blood amount 0-5), `cl_fx` (fx detail), `cl_motionblur`, `cl_wavedetail` (bullet
  trails) → gate particle/blood/blur/trail emission in the client FX paths.
- `cl_solidcross` (crosshair style 0-2), `cl_laser` (lasersight style 0-2 — **note we currently read
  the unrelated `cl_lasercolor`, not `cl_laser`**) → crosshair/laser rendering.
- `cl_showequipment` (equipment overlay transparency 0-3), `cl_thirdpersonswitch` (auto-3rd-person on
  kung-fu), `cl_thirdtransparency` (fade own 3rd-person model when aiming) → HUD/view.
- **Fix:** for each, cache the cvar pointer (as `ts_hud.cpp` already does for `hud_r/g/b` and
  `ts_attachments.cpp` for `cl_shadow`) and branch on `->value` at the corresponding render site. This
  requires per-feature RE of the original `client.dll` to match exact thresholds — out of scope for
  this audit; flagged as follow-on. // VERIFY each against `reference/client.dll`.

### 3. `mp_teammodels` registered but not read (team mode cosmetic)
- Registered `ts_cvars.cpp:53` (var `teammodels`), never read. Original `g_teamModels` drives
  per-team player-model assignment in TeamDM/The-One. Out of the current slice (team model
  assignment); low priority. **Fix:** read `CVAR_GET_STRING("mp_teammodels")` where teams pick models
  (alongside the `mp_teamlist` consumer in `teamplay_gamerules.cpp`).

### 4. `cashforwin` / `cashforlose` registered but not read (usecash round economy)
- Registered `ts_cvars.cpp:55/56`, never read; both are live `.data` cvars in the original and appear
  in multiple string refs (round win/lose cash award under the `usecash` economy). Only matters when
  `usecash` is on. **Fix:** award `cashforwin`/`cashforlose` at round end in the cash-DM path — but
  that whole economy loop is still a SLICE, so this rides with that work.

---

## Coverage summary
- **39 cvars** total in the mapping: **20** SERVER_OPTIONS + **19** INFO_OPTIONS.
- **Server gameplay (20):** 17 honored; **1 missing gameplay gap** (`globalslow` command);
  `mp_teammodels` missing-read (cosmetic); `superjump` and `sv_sendvelocity` are non-DLL / dead
  entries where our silence = parity. (`cashforwin`/`cashforlose` also unread but are extra cvars not
  in the `.scr` — noted for completeness.)
- **Client display (19):** 8 honored; **9 registered-but-unread** (`cl_showequipment`,
  `cl_thirdpersonswitch`, `cl_thirdtransparency`, `cl_fx`, `cl_motionblur`, `cl_gore`,
  `cl_wavedetail`, `cl_solidcross`, `cl_laser`); `mp_decals`/`cl_bob` engine-handled.
- **Priority verdict:** Starting Credits ✅ honored, Weapon restrictions ✅ honored (as command),
  Bullet physics detail ✅ honored server-side (Low = hitscan; boolean gate matches original — the
  user's Low setting IS respected). The one true server-gameplay miss is **`globalslow`** (fix 1).
