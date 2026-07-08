# cont.94 parity sweep — coverage + audit at HEAD (04a6a23, restructure-source-parity)

Standing structural task (HANDOFF OPEN #3). Libs rebuilt at HEAD via `scripts/build.sh amd64 all`
(hl.so was already current — no source drift since its 09:09 build; client.so relinked; glibc floor 2.14).
Regenerated: `reference/coverage/REPORT.md` (external contract), `AUDIT.md` (internal structure),
`RECONSTRUCTION_COVERAGE.md` (per-class method completeness). i386 build NOT rebuilt — only
`client_members.py` (a Ghidra pipeline, off-limits this sweep) consumes it.

NOTE per HANDOFF cont.93: the 13 local commits are RE-grounded but almost none playtest-confirmed.
This sweep reports STRUCTURE only; unconfirmed ≠ broken.

## Headline numbers

| Report | Result |
|---|---|
| REPORT.md (external) | msgs: 69 matched / **7 missing** / 4 extra · wire sizes **0/64 mismatched** · events 12/12 · keyvalues 0 unhandled · server cmds 0 missing / **15 extra** · client cmds **1 missing** / 8 extra · client cvars **11 missing** / 21 extra · server cvars 0 missing / 258 extra (252 = stock `sk_*`) · cldll exports **1 missing** (HUD_DirectorEvent) · 0 cvar-default mismatches |
| AUDIT.md (internal) | 0 missing / 0 invented classes · 0 missing methods · **23 invented methods** · 0 missing members · **1 missing / 6 invented globals** · 0 file mismatches |
| RECONSTRUCTION_COVERAGE.md | claims 401/474 (84%) — **misleading, see Tooling §**; true method gap after cross-check is ~5 named methods (~99%) |

## Corrections established this sweep (don't re-derive)

1. **reconstruction.py's 0% rows are a tooling artifact.** Its "ours" side filters classes by
   `TS_RE` (CTS*/CVip/CM61/...), so TS methods added to STOCK classes (CBasePlayer 0/40,
   CBaseEntity 0/10, CFuncPlat, CHalfLifeMultiplay, CBaseDoor, CBasePlayerWeapon, CBreakable,
   CMultiManager, CLadder, CRotDoor, CTriggerPush) always read 0 even though nm proves they exist
   (e.g. `CBasePlayer::DropWeapon`, `::GoSlow`, `::TSInit` all in our hl.so). Cross-checking every
   "missing" name against `nm hl.so | c++filt` leaves only these TRUE gaps:
   - `CTSGun::Holster(int)` — real (item 9 below)
   - `CBasePlayer::GiveSlowMul` — orig @0x82960 is **byte-identical** to `CBaseEntity::GiveSlowMul`
     (both `FixedSlowAmt(pev->fuser1)`, pev+0x254 = fuser1); behaviorally covered by our cbase.h
     inline. Name-parity nit only.
   - `CHalfLifeMultiplay::GetNextBestWeapon` @0xbf418 (0x181 bytes) — call graph is stock (2×
     `SwitchWeapon`, nothing TS); halflife-updated moved this logic into `CGameRules::GetNextBestWeapon`
     which we have. SDK divergence, not a TS gap.
   - `CLadder::KeyValue` @0xd8df8 / `CTriggerPush::KeyValue` @0xd8f20 — present in the 2003 stock
     SDK, removed by halflife-updated. Likely divergence noise; FGD keyvalue check shows 0 unhandled
     keys, so no map-facing gap. Verify once against stock-2003 source, then drop from the list.
   - `CMultiManager::IsClone/ShouldClone` — exist inline in dlls/triggers.cpp:288 (invisible to nm).
   - `__user_type_info::*` — gcc2 RTTI junk, ignore.
2. **REPORT.md's entity-factory section is FGD-based and overstates both directions.** Binary-level
   diff (orig `T` lowercase factories vs our nm) = **0 missing**, and ours-only = just `grenade` +
   `testhull` (plus libiberty `d_*` demangler noise from `__cxa_demangle` in our .so). The 62 "extra"
   (weapon_gauss, xen_*, bodyque, …) all exist in the original binary too; the 4 "missing"
   (func_mortar_field, info_bigmomma, info_player_coop, item_airtank) are NOT in the original binary
   either (FGD base-class inheritance).
3. **The `x` "extra user message" is a false positive** — the scoreboard close-button label string
   `CommandButton("x", …)` in cl_dll/vgui_ScorePanel.cpp:225.
4. All 7 "missing" user messages are **already registered by our server** with correct names/sizes
   (UserMessages.cpp); the gap is 100% client-side (no HOOK_MESSAGE/handler).

---

# PRIORITIZED WORK-LIST

## A. External contract (drop-in breakers) — do first

1. **Client hooks + HUD for the 7 TS user messages** (vs an original SERVER our client silently drops
   these = whole features dark):
   - `TSMBlur` (size 2; server sends from dlls/ts_player.cpp:388 UsePowerUp) → client motion-blur wash.
     Pairs with open cvar `cl_motionblur` (HANDOFF #6).
   - `NextStunt` (1; ts_stunt.cpp — our sender is DELIBERATELY parked "no client handler yet") → stunt meter.
   - `TSPSys` (-1; ts_sharedemitter.cpp) → client particle systems.
   - `TSFlag` (4; ts_flag.cpp gmsgTSHaveFlag) → flag-carry HUD.
   - `Objective` (-1) + `ShowObj` (2; TS_gamerulesclass.cpp) → objective HUD entries.
   - `TSHVoice` (1; TS_objectives.cpp) → capture taunt voice.
   Fix direction: RE the original client handlers (client.dll MsgFunc_*; dc.sh client once Ghidra frees
   up) and implement the CHud* consumers; unpark `TS_SendNextStunt`.
2. **11 missing client cvars** — `gl_fog, gl_fog_red, gl_fog_green, gl_fog_blue, gl_fog_start, gl_fog_end`
   (the CTSFog consumer — fog is currently unrenderable client-side), `cl_drawnames`, `cl_thirdperson`,
   `m_accel, m_accelexp, m_accelmax`. Register + wire (merge with HANDOFF OPEN #6's 9 registered-but-unread
   display cvars — same client.dll RE pass).
3. **`specmode` mechanism swap** — the original registers `specmode` (and `special`) as CLIENT commands
   (pfnAddCommand, original client_commands list); we instead handle them in the SERVER's ClientCommand
   (dlls/client.cpp:788/:804, HL-leftover branches) and never register client-side. Per the
   mimic-original-mechanism law: add the client registration, remove the server strcmp branches.
   (`+break/-break`, `+alt1/-alt1` flagged server-extra likewise — the original has them client-side only.)
4. **cldll export `HUD_DirectorEvent`** — original client.dll exports it; we export the SDK-renamed
   `HUD_DirectorMessage`. Rename/alias ours (cheap; keeps the engine→client spine byte-faithful).
5. **15 extra server ClientCommand branches** (dlls/client.cpp): HL leftovers `spectate, follownext,
   menuselect, closemenus, give, vote, special, specmode` + invented dev stand-ins `tsbuy, tsbuyammo,
   tsbuyarmament` (flagged "SLICE" in-source; original buys go through the client `buy` menu + `weapon_%d`
   args). Remove leftovers; replace tsbuy* with the original's dispatch when the buy-menu wire is finished.
6. **Extra client hooks `ResetFade, SpecFade, ViewMode`** — HL spectator leftovers, remove the
   HOOK_MESSAGEs (original client never hooks them). (`x` = false positive, ignore.)
7. **Extra cvars** — client: 21 HL leftovers (`spec_*`, `m_customaccel*`, `cl_rollangle/rollspeed/bobtilt`,
   `cl_autowepswitch`, `hud_saytext`, …); server: `sk_*` ×252 (bulk stock skill registrations — one-line
   gate to strip), `allow_spectators, mp_flashlight, mp_teamlist, sv_allowbunnyhopping, sv_busters,
   sv_pushable_fixed_tick_fudge`. Drift markers, low behavioral risk — strip registrations for parity.

## B. INVENTED symbols (drift — each is a suspect logic placement; AUDIT.md §INVENTED)

8. **CTSGameRules ×11**: `AssignToTeam, GetIndexedTeamName, GetTeamID, IsTeamMode, IsTeamplay,
   PlayerKilled, PlayerSpawn, RecountTeams, SendRoundTime, StartRoundTimer, TeamWithFewestPlayers`.
   The original's 39 methods are all matched — so its team/round logic lives INSIDE those 39 (Think /
   InitHUD / ClientDisconnected …), not as separate methods. Decompile-diff each area and fold our
   helpers into the original's method bodies (the cont.93 "MOVE/RENAME, don't rewrite" archetype).
9. **CTSGun ×7**: `CanDeploy, LoadActiveFromItem, OwnedCount, SendWeaponAnim, SlotsUsed,
   SyncActiveToItem, UpdateActivePointers`. The park/restore + slot accounting we invented around the
   single-gun model; original does this inline in Deploy/HolsterWeapon/SelectWeapon/AddWeapon. Related
   small close: **`CTSGun::Holster(int)` @0x75674 is MISSING** — a 0x21-byte forwarder
   `{ HolsterWeapon(); }` (never calls the base). We have no override, so stock
   `CBasePlayerWeapon::Holster` runs on engine holster paths. Add the override — in BOTH libs
   (CTSGun is client-predicted; memory `dropin-both-directions`).
10. **CBasePlayer ×5**: `TSEnterSpectate, TSSendScoreInfo, TSSpectateThink, TSTryRespawn,
    TSUpdateClientData`. Original spectate/score/respawn flow lives in `StopObserver / Observer_Idle /
    UpdatePlayerState / UpdateClientData`. Fold in / rename to the original names.
11. **Globals** — INVENTED ×6: `g_TSGun, g_tsActive, g_tsLocalFov, g_tsPlayerGunAtt,
    g_tsPlayerGunAttTime, g_tsPool`; MISSING ×1: **`TSGameRules`** (original: a global variable in
    TS_player.cpp; ours: an inline function `TSGameRules()` in ts_gamerules.h:216). Make it a real
    global (or document the accepted divergence) and re-home the g_ts* state onto original members
    (memory: members/globals signal hidden logic).

## C. Small closes / parity nits

12. `CBasePlayer::GiveSlowMul` — add the trivial override (byte-identical to base; name parity only).
13. `grenade` + `testhull` LINK_ENTITY_TO_CLASS — the original binary does NOT link either (TS removed
    them; `bodyque` it kept). Remove ours for exact factory parity.
14. `CLadder::KeyValue` / `CTriggerPush::KeyValue` — restore the 2003-stock overrides if a
    divergence.py check shows behavior differs; else record as accepted SDK divergence.
15. `CHalfLifeMultiplay::GetNextBestWeapon` — verified stock-shaped (calls only SwitchWeapon);
    record as accepted SDK-refactor divergence (logic lives in our CGameRules base method).

## Tooling fixes for the next sweep

- **reconstruction.py**: drop the `TS_RE` filter on the OURS side (match any class name present in the
  work-list) and resolve inline-defined methods via srcparse like audit.py does — the 84% headline is
  really ~99% and the report currently manufactures 11 fake 0% classes.
- **coverage.py factories**: add the binary-level factory diff (orig `T` lowercase vs ours) alongside
  the FGD view, and filter libiberty `d_*`/demangler symbols from ours.
- **audit.py blind spots hit**: 48 client.dll methods remain UNVERIFIABLE (no original client symbols —
  CHudTSBuyMenu/CHudTSWeaponBar accepted-by-decision C19); the historical ".h-only member scan" gap is
  FIXED (AUDIT.md now scans .cpp+.h brace-aware). The `x` user-message false positive is a coverage.py
  string-scan artifact worth suppressing.

## Verification pointers

- orig addresses: `Holster__6CTSGuni` @0x75674 (size 0x21) · `GiveSlowMul__11CBasePlayer` @0x82960 ·
  `GetNextBestWeapon__18CHalfLifeMultiplay…` @0xbf418 (0x181) · `KeyValue__7CLadder…` @0xd8df8 ·
  `KeyValue__12CTriggerPush…` @0xd8f20 · `FixedSlowAmt__Ff` @0x890bc · `bodyque` @0xe3b80.
- our senders for the 7 msgs: dlls/UserMessages.cpp (registrations), ts_stunt.cpp (NextStunt, parked),
  ts_player.cpp:388 (TSMBlur), ts_sharedemitter.cpp (TSPSys), ts_flag.cpp (TSFlag),
  TS_gamerulesclass.cpp:118 (ShowObj/Objective).
