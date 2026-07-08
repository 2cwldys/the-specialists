# cont.94 — server-strip safety verification (read-only sweep)

Verification agent, branch `restructure-source-parity`. Tools: nm/objdump/readelf/rz-bin/strings/grep
only (no Ghidra — projects owned by other agents). Law applied: EXTRA/INVENTED = drift to remove,
but never break a live loop. GOT base for all `ts_i386.so` xrefs: `0x165e10` (string/global operand
= `vaddr − 0x165e10` as the `lea/mov -0xNNN(%ebx)` displacement).

---

## TASK 1 — the 252 `sk_*` cvars: SAFE-STRIP (with 4 must-replace value fixes)

### The original's mechanism (verified in the binary)

- **Zero `sk_*` strings** in `ts_i386.so` (`rz-bin -zz` + `strings -a | grep`): the original
  registers NO skill cvars and never reads one by name.
- **`GetSkillCvar__FPc` @0xce9cc exists but is DEAD** — linked in from skill.cpp, zero callers
  (the only `call` near it is its own PIC thunk). The `"** GetSkillCVar Got a zero for %s **"`
  string survives unreferenced-by-callers. TS kept skill.cpp compiled but disconnected.
- **`CGameRules::RefreshSkillData` @0xb5fd8 is 0x78 bytes**: `CVAR_GET_FLOAT("skill")` (string
  @0xfd05c) → store UNCLAMPED to `g_iSkillLevel` (GOT 0x13d8) → clamp 1..3 → store to
  `gSkillData.iSkillLevel` (GOT 0x1b2c, offset 0). **No GetSkillCvar calls, no field fill, no
  ALERT print.** Every other gSkillData field stays 0 (BSS).
- **`CHalfLifeMultiplay::RefreshSkillData` @0xbef78** calls the base then HARDCODES exactly 15
  fields (skilldata_t offsets → values, IEEE-decoded):
  | off | field | value |
  |---|---|---|
  | 0x118 | suitchargerCapacity | 30 |
  | 0xc8 | plrDmgCrowbar | 25 |
  | 0xcc | plrDmg9MM | 12 |
  | 0xd0 | **plrDmg357** | **40** |
  | 0xd4 | plrDmgMP5 | 12 |
  | 0xd8 | plrDmgM203Grenade | 100 |
  | 0xdc | plrDmgBuckshot | 20 |
  | 0xe0 | plrDmgCrossbowClient | 20 |
  | 0xe8 | plrDmgRPG | 120 |
  | 0xf0 | plrDmgEgonNarrow | 10 |
  | 0xf4 | plrDmgEgonWide | 20 |
  | 0xf8 | plrDmgHornet | 10 |
  | 0xfc | plrDmgHandGrenade | 100 |
  | 0x100 | plrDmgSatchel | 120 |
  | 0x104 | plrDmgTripmine | 150 |
  (NOT set: 0xe4 crossbowMonster, 0xec gauss — same as stock-2003 MP override.)
- **Complete original gSkillData reader set** (every `0x1b2c(%ebx)` GOT load in the disasm):
  `CBaseMonster::TraceAttack` (5×, monHead/Chest/Stomach/Arm/Leg — all 0 in the original),
  `CBaseEntity::FireBullets` (3×, monDmg*), `CBaseEntity::FireBulletsPlayer` (4×, plrDmg*),
  the two RefreshSkillData, GetSkillCvar. **Nothing else.** In particular the item/charger code
  does NOT read gSkillData:
  - `CHealthKit::MyTouch` @0xb8968: `TakeHealth(50.0, 0)` — immediate `0x42480000`.
  - `CItemBattery::MyTouch` @0xbb74c: gates on `pev->armorvalue < 100.0` (const @0xf761c) then
    **`TakeArmor(player, 50)`** (`push $0x32` → `TakeArmor__11CBasePlayeri` — the TS kevlar
    method we already have at dlls/ts_buy.cpp:86), then sound + a msg with cap 100.
  - `CWallHealth::Spawn/Recharge` @0xb8c1c/@0xb8f2c and `CRecharge::Spawn` @0xb6ee4:
    **`m_iJuice`(this+0x10c) = 1** hardcoded (both Spawn and Recharge), no gSkillData.
- `"exec skill.cfg"` @0xfcfc4 IS original — referenced from **`GameDLLInit__Fv`**. (ts/ ships no
  skill.cfg; the valve-fallback one execs `sk_*` sets that hit unregistered cvars — same
  harmless console noise as the original.)

### Our TS-live exposure (every `gSkillData.` read in dlls/, classified)

- `combat.cpp:1449–1566` FireBullets/FireBulletsPlayer switch (plrDmg9MM/MP5/Buckshot/357,
  monDmg*) — **same reads as the original**; our TS guns pass explicit `iDamage` (server path is
  `ShootBulletsPlayer` with wi->flDamage; the `BULLET_PLAYER_9MM` FireBulletsPlayer calls in
  ts_gun.cpp:1143/2323 are the CLIENT-prediction branch with nonzero damage) → the gSkillData
  switch arms only matter for stock weapons, covered by the MP hardcode. SAFE.
- `combat.cpp:1334–1348` CBaseMonster::TraceAttack monHead/monChest/… — reads exist in the
  original too, where they evaluate to 0 (never filled). Monsters aren't in TS MP. SAFE (0 = parity).
- `ggrenade.cpp:398` plrDmgM203Grenade — filled by the MP hardcode (100). SAFE.
- `h_battery.cpp:96,185` suitchargerCapacity — filled by the MP hardcode (30). SAFE.
- `multiplay_gamerules.cpp:128–168` — the hardcode block itself (writer). KEEP.
- `stats.cpp` AmmoDamage — **zero callers in our tree** and `AmmoDamage`/`UpdateStats` have NO
  symbols in ts_i386.so → dead stock file (separate strip candidate, out of scope).
- **MUST-REPLACE (TS-live: all four entities are in `reference/ts_fgd.fgd` — map-placeable):**
  - `healthkit.cpp:73` `TakeHealth(gSkillData.healthkitCapacity, …)` → **`TakeHealth(50, DMG_GENERIC)`**
    (original @0xb8968). NB: today ours heals 15 (valve skill.cfg) — this is an ACTIVE divergence
    the strip fixes.
  - `items.cpp:228` `pev->armorvalue += gSkillData.batteryCapacity` → the TS shape:
    `armorvalue < 100` gate + **`pPlayer->TakeArmor(50)`** (original @0xbb74c).
  - `healthkit.cpp:167,243` `m_iJuice = gSkillData.healthchargerCapacity` → **`m_iJuice = 1`**
    (original @0xb8c1c/@0xb8f2c).
  - `h_battery.cpp` juice is suitchargerCapacity=30 (hardcode) — no change needed, but note the
    original CRecharge::Spawn also stores `m_iJuice = 1` @0xb6f6e; VERIFY once in-game whether
    suit chargers should be 30 (our MP hardcode) or 1 (the per-entity store). Low stakes.

### The strip (exact edits)

1. `dlls/game.cpp`: delete the 252 `cvar_t sk_*` definitions (lines ~54–455) and the 252
   `CVAR_REGISTER(&sk_*)` calls (lines ~635–1036, the "REGISTER CVARS FOR SKILL LEVEL STUFF"
   block). No header externs exist (`extern cvar_t sk_` count = 0); no other file references any
   `sk_*` cvar_t. **KEEP** `SERVER_COMMAND("exec skill.cfg\n")` (original, in GameDLLInit).
2. `dlls/gamerules.cpp` `CGameRules::RefreshSkillData` (~line 185): delete everything after the
   `gSkillData.iSkillLevel = iSkill;` store — all ~100 `GetSkillCvar("sk_…")` assignments AND the
   `ALERT(at_console, "\nGAME SKILL LEVEL:%d\n")` (original has no print). Resulting body =
   read/store-unclamped/clamp/store, exactly the original's @0xb5fd8.
3. **KEEP** `dlls/skill.cpp`/`GetSkillCvar` compiled (the original links it dead — name parity).
4. `dlls/multiplay_gamerules.cpp:137`: **`plrDmg357 = 50` → `40`** (original @0xbefb8 =
   0x42200000). The only value mismatch in the block; everything else matches the binary.
5. Apply the three MUST-REPLACE hardcodes above (healthkit 50 / battery TakeArmor(50) /
   healthcharger juice 1).

Risk after strip: every formerly-cvar-fed field reads 0 — which is byte-what the original does.

---

## TASK 2 — extra ClientCommand branches (dlls/client.cpp): per-command verdicts

Ground truth — the ORIGINAL `ClientCommand__FP7edict_s` @0x9d838 dispatch, fully enumerated by
resolving every GOT string lea in the function:
`say, say_team, fov ("\"fov\" is \"%d\"\n"), cashala, lasclr_, rst_cfg, tkw(+"%i_%i"), tki, tka,
cutwire_, drop, drop_, usepowerup, powerup, firemode, useitems, lastinv, changeteam, jointeam,
motd, fullupdate, respawn, use, weapon_, mj(+"1" physinfo), "Unknown command: %s\n"`.
(NB: rz-bin's default min string length hid the 3-char `tkw/tki/tka` — they ARE original, at
0xfb696/0xfb6a0/0xfb6a4. Earlier "not in binary" reads of 3–4-char commands are unsafe; use the
dispatch-lea enumeration.)
`menuselect` @0xfd78f is referenced ONLY by `ClientCommand__18CHalfLifeMultiplay` +
`ClientCommand__17CHalfLifeTeamplay`. `vote` @0xfb632 is referenced ONLY by `Host_Say__FP7edict_si`.

| cmd | in orig server dispatch? | who sends (ours) | verdict |
|---|---|---|---|
| `spectate` | NO | cl_dll vgui_teammenu.cpp:150 + vgui_TeamFortressViewport.cpp:943 (team-menu button); orig client.dll has NO `spectate` string | **STRIP BOTH SIDES** — server branch (client.cpp:773) + the two VGUI senders. Also removes the only reader of extra cvar `allow_spectators` (game.cpp:46/597 → strip it too). TS spectate is the ts_spectate/StartObserver flow, not this HL leftover. |
| `follownext` | NO | cl_dll hud_spectator.cpp:914 (stock HL spectator); orig client.dll: no string | **STRIP BOTH SIDES** — server branch (client.cpp:795) + the sender (original spectator control is client-local `specmode`/HLTV, dossier cont94 §9). |
| `menuselect` | NO in client.cpp-equivalent — **YES in gamerules** (CHalfLifeMultiplay + CHalfLifeTeamplay ::ClientCommand) | cl_dll menu.cpp:228 `"menuselect %d"` — orig client.dll sends the SAME (`menuselect %d` string present) | **MOVE, don't strip**: delete the client.cpp:707 branch; add the parse-and-discard branch to `CHalfLifeMultiplay::ClientCommand` (dlls/multiplay_gamerules.cpp:105, after the VoiceGameMgr check). Keep the cl_dll sender (matches orig client). CTSGameRules inherits it → same reachability. |
| `closemenus` | NO | cl_dll vgui_TeamFortressViewport.cpp:1368; orig client.dll: no string | **STRIP BOTH SIDES** — server no-op branch (client.cpp:791) + the sender. |
| `give` | NO | nobody in cl_dll; **xash engine forwards it** (cl_main.c:3627 `Cmd_AddCommand("give", NULL,…)` = forward-to-server) | **STRIP** (client.cpp:736). After the strip a xash `give` prints "Unknown command: give" — exactly what the original server does. Dev-cheat loss only (`powerup`/`cashala`/`impulse` remain). |
| `vote` | NO — original parses it in **Host_Say** | users typing `vote …` in console (engine forward) | **MOVE the mechanism**: delete the client.cpp:811 branch. In our `Host_Say` (client.cpp:380), mirror @0x9d46c: if `!strncmp(p, "vote", 4)` → `strlwr(p+4)`, copy, gate on the player's vtable-0xcc check (IsAlive-shaped), call `TSGameRules PlayerVote(pEntity, text_after_vote)`; nonzero return SUPPRESSES the chat broadcast. Our `CTSGameRules::PlayerVote(edict_t*, const char*)` (TS_gamerulesclass.cpp:547) already matches the original signature `PlayerVote__12CTSGameRulesP7edict_sPc`. Net effect: voting via chat ("say vote map x"), console `vote` → Unknown command, both = original. |
| `+alt1/-alt1/+break/-break` | NO (falls to Unknown command) | key binds (engine forward — our cl_dll registers none of them); orig client.dll REGISTERS all four (`+alt1`,`-alt1`,`+break`,`-break` strings present) | **STRIP the server accept-branch (client.cpp:800) + ADD the four client-side `pfnAddCommand` registrations** in cl_dll (same pattern as the cont94 `specmode` fix in hud_spectator.cpp:207). Until their behavior is RE'd, empty handlers client-side = original wire silence. |
| `special`/`specmode` | NO | — | **ALREADY DONE** post-sweep (commit 6f7b147; comments at client.cpp:788/:804 area). No action. |
| `tsbuy` | NO (invented name) | nobody (console-only dev stand-in) | **SAFE-STRIP** (client.cpp:659). Original has NO direct-buy command: `BuyWeapon__11CBasePlayer` @0x845bc is called ONLY from inside `BuyArmament` @0x83f10. |
| `tsbuyammo` | NO | nobody | **SAFE-STRIP** (client.cpp:664). `BuyAmmo__11CBasePlayeri` @0x841e8 likewise called only from BuyArmament. |
| `tsbuyarmament` | NO | nobody | **SAFE-STRIP** (client.cpp:701). The original's ONLY BuyArmament caller is **`CHalfLifeMultiplay::PlayerSpawn` @0x790ac (call @0x79122)** — loadout applies on spawn. Ours already mirrors this: `CTSGameRules::PlayerSpawn` → `pPlayer->BuyArmament()` (TS_gamerulesclass.cpp:1070). This also CLOSES the BUY.md open question: there is no separate apply-trigger command; apply = spawn. |

Buy-protocol coverage check (why tsbuy* loses nothing): cl_dll/ts_buymenu.cpp already emits the
original wire — `rst_cfg` (:483), batched `tkw` (:528), `tka` (:546), `tki 16` (:549) — and the
server handles tkw/tki/tka/rst_cfg/cashala (all confirmed original dispatch strings). Queue →
spawn-apply loop is closed. Lost capability: mid-life instant buy from console — NOT an original
capability (dev convenience only).

Suggested BUY.md edit alongside: replace "exact apply-trigger command is still TBD" with the
@0x79122 PlayerSpawn finding.

---

## TASK 3 — `grenade` / `testhull` LINK_ENTITY_TO_CLASS: SAFE-STRIP (both)

- Original exports: `nm -D ts_i386.so` has **no `grenade`, no `testhull`** factory; `bodyque`
  @0xe3b80 it kept (ours kept too — dlls/world.cpp:203 — no change).
- Our LINK sites: `dlls/ggrenade.cpp:33` `LINK_ENTITY_TO_CLASS(grenade, CGrenade)`;
  `dlls/nodes.cpp:1485` `LINK_ENTITY_TO_CLASS(testhull, CTestHull)`.
- Name-based spawn audit: **no `CBaseEntity::Create("grenade")`/`Create("testhull")`/
  `CREATE_NAMED_ENTITY` by these names anywhere in dlls/.** All internal spawns are
  `GetClassPtr((CGrenade*)NULL)` (ggrenade.cpp:379/406/447), `GetClassPtr((CM61Grenade*)NULL)`
  (ts_grenades.cpp CreateGrenade — sets `pev->classname = MAKE_STRING("grenade")` directly, no
  factory), and `GetClassPtr((CTestHull*)NULL)` (nodes.cpp:1569). The
  `FIND_ENTITY_BY_CLASSNAME(…, "grenade")` scans (ggrenade.cpp:488/502) match the classname
  STRING, which the direct `pev->classname` stores still set — unaffected by factory removal.
- `m61_grenade` keeps its LINK (the original exports that factory @0x6c850 — ts_grenades.cpp:305).

**Edit:** delete the two LINK lines (ggrenade.cpp:33, nodes.cpp:1485). Nothing else changes.
Factories are only consumed by engine name-spawn (maps / CREATE_NAMED_ENTITY); no TS map or code
path uses either name.

---

## TASK 4 — §B11 globals: classification

**MISSING `TSGameRules` (server) — NOT mechanical; architectural, defer to fold-planner.**
Binary facts: `TSGameRules` is a **22,676-byte OBJECT in .bss @0x1681a0** — the CTSGameRules
INSTANCE itself, constructed by `__static_initialization_and_destruction_0` (static init, file
TS_player.cpp per AUDIT) — not a pointer. It COEXISTS with `g_pGameRules` (4-byte D @0x10904c):
the original `InstallGameRules__Fv` @0xb6050 still `new`s stock rules objects (CHalfLifeRules
0x4(!) / CHalfLifeTeamplay 0x218 / CHalfLifeMultiplay 0xc(!) bytes — gutted shells; the real
state lives in the static TSGameRules). 50+ original functions touch it via GOT 0x14f0
(PreThink, UpdateClientData, PlayerKilled, CaptureTouch, cmd_globalslow, Host_Say, …).
Ours = single `new CTSGameRules` installed AS g_pGameRules + inline accessor `TSGameRules()`
(ts_gamerules.h:216, 65 call sites). Converting to a static object changes lifecycle semantics
(ctor at dlopen vs per-InstallGameRules) — a redesign, not a rename. Cheap intermediate if the
audit gap must close early: a real global `CTSGameRules* TSGameRules` set in InstallGameRules +
sed `TSGameRules()->` → `TSGameRules->` (mechanical, 65 sites) — but it would be a pointer where
the original has an object; fold-planner should decide object-vs-pointer with the InstallGameRules
gutting in view.

**INVENTED ×6 — all are CLIENT-side (cl_dll), where the original client.dll has no symbols
(AUDIT's "48 methods UNVERIFIABLE" caveat applies to globals too):**
- `g_TSGun` (cl_dll/hl/hl_weapons.cpp:55) — the client-predicted CTSGun instance. Stock-SDK
  precedent (hl_weapons.cpp's file-scope weapon objects). Load-bearing for prediction.
  **Defer/accept** until client RE names the original's counterpart.
- `g_tsActive` (cl_dll/ts_weapon_predict.cpp:25) — active-weapon cache feeding the event
  handlers (ev_ts.cpp:849/970). Load-bearing invention; the original fire-handler's state model
  is un-RE'd. **Defer to fold-planner.**
- `g_tsLocalFov` (cl_dll/entity.cpp:74 → ts_scope.cpp) — clientdata→scope fov conduit. Small and
  mechanical to re-home (e.g. a CHud member), but there is no VERIFIED original target — moving it
  onto an invented member just relocates the flag. **Defer/accept.**
- `g_tsPlayerGunAtt` + `g_tsPlayerGunAttTime` (cl_dll/StudioModelRenderer.cpp:24/25) — per-entity
  gun-attachment cache for laser/flashlight anchors (ts_attachments.cpp staleness check).
  Mechanism invention; load-bearing for attachment rendering. **Defer to fold-planner** (original
  likely reads `cl_entity_t::attachment` at use-time — needs client RE).
- `g_tsPool` (cl_dll/ts_particles.cpp:54, already `static`) — sprite pool for our particle
  system. The original's TSPSys client handler is itself still MISSING (sweep §A1) — restructure
  it WHEN TSPSys is RE'd, not before. **Defer.**

Bottom line: 0 of the 6 are mechanically fixable against verified original structure today; the
one server-side item (TSGameRules) is concrete but architectural. None should block the strips.

---

## Bonus divergences found while verifying (not in the sweep)

1. `multiplay_gamerules.cpp:137` **plrDmg357 50 → 40** (binary @0xbefb8). One line.
2. **healthkit heals 15 today** (valve skill.cfg via sk_healthkit) **vs original 50** — the strip
   + hardcode FIXES an active behavior divergence, not just structure.
3. Ours prints `"GAME SKILL LEVEL"` ALERT in RefreshSkillData; original doesn't.
4. `dlls/stats.cpp` (AmmoDamage/UpdateStats) is dead in our tree AND absent from the original
   binary — candidate for removal from the 4 build source lists (run scripts/check-source-lists.py).
5. Duplicate dead branches in our ClientCommand: second `drop` (client.cpp:745) and second
   `lastinv` (client.cpp:769) are shadowed by the TS branches at :653/:607 (first match wins) —
   fold into the strip.
