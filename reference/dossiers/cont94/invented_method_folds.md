# cont.94 — invented-method fold plan (parity_sweep §B: 23 methods + the TSGameRules global + 6 client globals)

READ-ONLY planning dossier. Branch `restructure-source-parity`. Evidence base:
- **cont.84 `dossier-invented.md`** (this directory's sibling `../cont84/`) + its raw decompiles
  (`inv_A*.c`, `inv_BC_*.c`, `inv_C_*.c`, `inv_D_*.c`) — the per-method original-home RE. All homes
  re-verified this session against the CURRENT tree (cont.84 predates several closes; its
  TSSendBuyMenu/TSMenuSelect/TSCloseMenu/GetWeaponData/SendWeaponStatus items are ALREADY DONE and
  are not in the current 23).
- Fresh disasm verifications done THIS session (cited inline): `lastinv` dispatch @0x9e3bc–0x9e454,
  `CTSGun::GetItemInfo` @0x75dd0, death-scatter callers of `ThrowWeapon`/`DropWeapon` (all inside
  `CBasePlayer::Killed` @0x7ed5c, sites 0x7f085–0x7f185), `TogglePlayerFromSlow` caller census
  (ZERO callers in the original — dead code), `__vt_` census (CTSGameRules has NO vtable),
  `ts_inventory.json` ancestry (`CTSGameRules.base = null`).
- NB: the working tree is receiving the server_strips edits concurrently (gamerules.cpp
  RefreshSkillData already stripped). Line numbers below were taken at HEAD 04a6a23 + strips; the
  apply agent should re-anchor by the quoted code, not blindly by line.

---

## 0. THE ARCHITECTURE FINDING (drives every grade below)

**The original `CTSGameRules` is a STANDALONE class — `base: null`, no vtable, all 39 methods
non-virtual** (`nm` has no `__vt_12CTSGameRules`; `ts_inventory.json` ancestry = `["CTSGameRules"]`).
It is the 22,676-byte static OBJECT `TSGameRules` @0x1681a0 (server_strips TASK 4), reached via GOT
`PTR_TSGameRules`. It **coexists** with `g_pGameRules`, which holds a stock (TS-patched)
`CHalfLifeMultiplay` / `CHalfLifeTeamplay` instance. So in the original:

- virtual gamerules hooks (`PlayerSpawn`, `PlayerKilled`, `IsTeamplay`, `GetTeamID`,
  `GetIndexedTeamName`, `ClientCommand`, …) run on **CHalfLifeMultiplay/CHalfLifeTeamplay**
  (TS-patched in `TS_multiplay_gamerules.cpp` / teamplay);
- TS round/team/objective state lives on the **separate non-virtual TSGameRules object**.

Ours merges both into one `class CTSGameRules : public CHalfLifeMultiplay` installed as
`g_pGameRules`. That merge is exactly why the 11 CTSGameRules "invented methods" exist: each is
either (a) a virtual override standing in for the original's TS-patched CHalfLifeMultiplay/Teamplay
method, or (b) a helper the original does inline. **Full parity ultimately requires the two-object
split** (stock rules objects + standalone TSGameRules) — that is the one RED umbrella item (R1).
Everything gradeable below is graded per-method against what can be done WITHOUT the split.

---

# GREEN — pure moves, apply without re-RE'ing

### G1. `TSGameRules` global (closes AUDIT's 1 MISSING global) — the mechanical interim
**Ours:** inline accessor `TSGameRules()` in `dlls/ts_gamerules.h:216-219`
(`static_cast<CTSGameRules*>(g_pGameRules)`), **65 call sites** (`grep -rn "TSGameRules()" dlls/`
= 67 hits incl. the definition + one comment).
**Original:** a global **object** `TSGameRules` in TS_player.cpp (static init) — the object-ness is
R1; the interim that closes the name gap is a real pointer global.
**Edits:**
1. `dlls/ts_player.cpp` (the original's home file): add near the top globals block:
   `CTSGameRules *TSGameRules = nullptr; // interim: the original is a static OBJECT (see R1)`
2. `dlls/ts_gamerules.h:214-219`: replace the inline function with
   `extern CTSGameRules *TSGameRules;`
3. `dlls/gamerules.cpp` `InstallGameRules()`: in the CTSGameRules branch, after
   `CTSGameRules *pRules = new CTSGameRules;` add `TSGameRules = pRules;`; in the
   `CHalfLifeRules` / `CMultiplayBusters` branches add `TSGameRules = nullptr;`.
4. Mechanical sed across `dlls/`: `TSGameRules()->` → `TSGameRules->`, and the bare
   guard forms `if (TSGameRules())` / `TSGameRules() &&` / `TSGameRules() ?` → drop the parens.
   (No cl_dll sites exist.)
**Init-order hazard: none** — the accessor already resolved through `g_pGameRules`, which is set at
the same moment (world.cpp:527); nothing TS calls it pre-install. SAFER than today in the
`deathmatch 0`/busters branches: the old accessor returned a wrongly-cast non-TS object; the pointer
is honestly null there (sites that don't null-check were equally broken before — both paths are
outside TS's supported config).
**Grade: GREEN.**

### G2. `CBasePlayer::TSUpdateClientData` → inline into `CBasePlayer::UpdateClientData`
**Ours:** decl `dlls/player.h:475`; body `dlls/ts_player.cpp:59` (~250 lines of change-throttled
TSHealth/TSArmor/TSState/TSCash/TSSpace/PwUp/TSSlowMo/TSBTime/TSActiveItems/TSPAward/Wires sends);
single caller `dlls/player.cpp:4601` (end of `UpdateClientData`).
**Original:** it IS the TS block of `CBasePlayer::UpdateClientData` @0x81d00 (file home
**TS_player.cpp** per funcfile.tsv) — cont.84 §B1 documents the exact send order.
**Edits:** move the body of `CBasePlayer::TSUpdateClientData` (ts_player.cpp:59…end of function)
verbatim into player.cpp replacing the `TSUpdateClientData();` call at :4601 — OR (preferred,
matches the original file home) leave the code in ts_player.cpp but retitle the containing function:
it cannot stay a distinct method. Since our stock `UpdateClientData` must remain in player.cpp for
now (file-home divergence is a separate, known audit blind spot), the minimal pure move is:
**inline at the :4601 call site, delete player.h:475 + the ts_player.cpp:59 definition** (keep the
file's gmsg globals — they are original TS_player.cpp globals). Keep every comment.
**Behavior-neutral: yes** (same statements, same order, same single execution point).
**Grade: GREEN.** Follow-ups discovered (do NOT fold in, list in ledger): (i) the original's TS
sends are ORDERED differently (B1: InitHUD-block → HideWeapon → TSHealth → TSArmor → Train →
TSState → TSCash → TSSpace …) and are interleaved with the stock sends, not appended after them;
(ii) our UCD still ALSO sends stock `gmsgHealth`/`gmsgBattery` (player.cpp:4416/:4430) — the
original's UCD decompile has no stock Health/Battery sends (VERIFY, then strip); (iii) the original
UCD also sends the RoundTime catch-up in its InitHUD block (see G4 follow-up).

### G3. `CTSGameRules::SendRoundTime` → inline at its one caller
**Ours:** `dlls/TS_gamerulesclass.cpp:184-193`; single caller = `CTSGameRules::InitHUD`
(TS_gamerulesclass.cpp:1164).
**Original:** no such method — the joiner catch-up is INLINE in `CTSGameRules::InitHUD` @0x69134
(decompile `../cont84/inv_C_RoundTime_InitHUD_SetPlayerRespawnTime.c` lines 163-177):
gated `m_flRoundEnd != 0 && m_bRoundTimerEnabled(field_0x10)`, sends
`{BYTE round(max(0, m_flRoundEnd - time)), BYTE 0}` MSG_ONE.
**Edits:** replace the `SendRoundTime(pPlayer);` call at :1164 with the inline block, **using the
original's gate**, which differs from our helper's:
```
if (m_flRoundEnd != 0.0f && m_bRoundTimerEnabled)
{
    float left = m_flRoundEnd - gpGlobals->time;
    if (left < 0) left = 0;
    MESSAGE_BEGIN(MSG_ONE, gmsgTSRoundTime, NULL, pPlayer->edict());
    WRITE_BYTE((int)(left + 0.5f));
    WRITE_BYTE(0);
    MESSAGE_END();
}
```
Delete the helper (:181-193) + its decl (ts_gamerules.h:136).
**Divergence being fixed (flag):** our helper's gate was `m_flRoundEnd <= time → skip` and ignored
`m_bRoundTimerEnabled`; the original sends a CLAMPED 0 for an expired clock and honours field_0x10.
Also the helper's `gmsgTSRoundTime == 0` guard is ours (registration is unconditional — drop it).
**Grade: GREEN** (one site; decompile-exact replacement).
Follow-up (separate): the original ALSO sends this catch-up from `CBasePlayer::UpdateClientData`'s
InitHUD block (cont.84 B1 step 2, gated `TSGameRules->m_flRoundEnd != 0 && +0x10`). Ours doesn't.
Add when G2's block is touched next.

### G4. `CTSGameRules::StartRoundTimer` → inline at its two callers
**Ours:** `dlls/TS_gamerulesclass.cpp:33-45`; callers `RestartRound` (TS_gamerulesclass.cpp:393)
and `Think` (:711).
**Original:** no such method — the arm is INLINE in `RestartRound` @0x67920 (inv_C decompile:
`m_flRoundEnd = time + clamp(roundtime, 30, 180); m_flRoundRestart = 0;` then, if `field_0x10`,
broadcast `{BYTE secsLeft, BYTE 0}`). `Think` @0x77204 (cont91 `tsthink.c`) has **NO arm at all** —
first-round arming happens because `Init` @0x67d6c sets `m_flRoundRestart = time + 5` → Think fires
`RestartRound`.
**Edits (pure move):** inline the 13-line body at BOTH call sites verbatim; delete the method
(:33-45) + decl (ts_gamerules.h:135). At the RestartRound site, add the original's `if
(m_bRoundTimerEnabled)` gate around the broadcast (field_0x10 — our helper broadcast
unconditionally; RestartRound is the original's arm site so make that site decompile-exact).
At the Think:711 site inline verbatim WITHOUT behavior change and tag it:
`// INVENTED ARM (no original counterpart in Think @0x77204): kept because our first-round
// bootstrap doesn't run the original's Init(m_flRoundRestart=time+5) -> Think -> RestartRound
// chain yet. Remove when the Init/RestartRound bootstrap is made faithful. (fold-plan G4)`
**Grade: GREEN** (mechanical; the Think-arm divergence is pre-existing and explicitly preserved).

### G5. `CTSGun::UpdateActivePointers` → inline (both libs)
**Ours:** `dlls/ts_gun.cpp:384-389` (2 real lines); decl ts_gun.h:151. Call sites:
ts_gun.cpp:298 (Reset), :335 (Spawn), :674 (SelectWeapon), :702 (GetBetterWeapon), :2886
(GiveTSWeapon); cl_dll/hl/hl_weapons.cpp:65 (TS_SetPredictedWeaponType), :623 (prediction reseed).
**Original:** no helper — `Deploy` @0x71030 sets `m_pActiveItem = &items[type]` /
`m_pActiveInfo = &weapon_info[type]` inline (cont81 `deploy.c`), `HolsterWeapon` @0x75698 nulls
them inline (cont81 `holster.c`). cont.84 §A4 verdict + the Deploy caveat is now CLOSED (deploy.c
exists and confirms the inline-assignment shape).
**Edits:** at every call site replace `UpdateActivePointers();` /
`pGun->UpdateActivePointers();` / `g_TSGun.UpdateActivePointers();` with the two assignments,
receiver-qualified as needed:
```
m_pActiveInfo = get_weapon_info((tsweapon_t)m_weaponType);
m_pActiveItem = (m_weaponType > TSW_MELEE && m_weaponType < TSW_COUNT) ? &m_items[m_weaponType] : nullptr;
```
(the original guard is just "type != 0"; our `>= 0 && < TSW_COUNT && != TSW_MELEE` collapses to the
above since TSW_MELEE == 0 — keep the exact semantics of the current helper). Delete the method +
decl. **ts_gun.cpp is compiled into BOTH libs** (memory `dropin-both-directions`), so the two
hl_weapons.cpp sites are the only extra client edits.
**Grade: GREEN** (mechanical, 7 sites, no behavior change).

### G6. `CBasePlayer::TSTryRespawn` → inline into the `respawn` ClientCommand branch
**Ours:** decl `dlls/player.h:583`; body `dlls/player.cpp:1637-1655`; single caller
`dlls/client.cpp:621` (the `respawn` command branch).
**Original:** no such method — the gates live INLINE in the `"respawn"` branch of
`ClientCommand__FP7edict_s` @0x9d9fc (cont.84 §B2: requires `pev->iuser2 != 0`; `+0x770 < 0` →
1 Hz wait line; else `0 < +0x770 < time` → `+0x770 = 0; StopObserver(); pev->impulse = 0;`) — and
`StopObserver` @0x89a14 carries its OWN redundant elapsed-gate (decompile in
`../cont84/inv_BC_PlayerSpawn_Observers_PreThink.c` lines 140-175).
**Edits:** move the body of `TSTryRespawn` verbatim into client.cpp:621's branch (it is
`player->`-qualified there: `m_fInSpectateWait` → `player->m_fInSpectateWait` etc.); delete the
method + decl. Keep our comments (incl. the `iuser2` provenance note).
**Grade: GREEN** (pure move; single caller; the original's home IS a ClientCommand branch).
Follow-ups (YELLOW, don't do in the same edit): (i) gate on `pev->iuser2 != 0` instead of our
invented `m_fInSpectateWait` once iuser2 is verified set/cleared on our spectate path; (ii) add the
original's redundant guard INSIDE `StopObserver` (`m_flTSRespawnTime <= 0 || <= time` wraps the
whole body) — ours relies on the caller alone.

### G7. `CBasePlayer::TSSpectateThink` → inline into `PlayerPreThink`
**Ours:** decl `dlls/player.h:582`; body `dlls/player.cpp:1540-1594`; callers player.cpp:2031
(PreThink, gated `m_fInSpectateWait`) and :1531 (TSEnterSpectate's initial paint).
**Original:** no such method — it is the `pev->iuser1 != 0` block of `CBasePlayer::PreThink`
@0x808cc (region ~0x81458; decompile `inv_BC_PlayerSpawn_Observers_PreThink.c` lines 456-505:
the two 1 Hz HUD-message throttles off `+0x754`, the hold-window expiry, then
`Observer_HandleButtons(); Observer_Idle(); pev->impulse = 0; return;`). It contains **no
ScoreInfo/playerclass re-send** — that part of our body is invented (see Y6).
**Edits (pure move):** inline the :1540-1594 body at the player.cpp:2031 call site (replace the
call, keep the `return;`); at :1531 (inside TSEnterSpectate) DELETE the call — the original paints
nothing at spectate entry; the PreThink block paints on the next frame (≤0.1 s later, behaviorally
invisible; tag with `// fold-plan G7: the original has no entry-time paint`). Delete method + decl.
EXCEPTION: carry the `secs != m_iClientSpecSecs` ScoreInfo-reassert sub-block (:1555-1564) over
UNCHANGED for now — its removal is Y6's job, not this move.
**Grade: GREEN** (mechanical; the one dropped call is a ≤1-frame cosmetic).
Follow-up (YELLOW): the original gates on `pev->iuser1 != 0`, ours on `m_fInSpectateWait`; converge
when the spectate-state members are reconciled with iuser1/iuser2 (see Y7).

---

# YELLOW — signature/state/wire coupling; exact recipes, but verify at runtime

### Y1. `CTSGun::OwnedCount` — delete; fix the `lastinv` gate to the disasm-verified original
**Ours:** ts_gun.h:133, body ts_gun.cpp:340-347; single caller `TSLastInv` ts_gun.cpp:2930.
**Original:** no recompute helper exists (cont.84 §A3). The `lastinv` dispatch in
`ClientCommand__FP7edict_s` — **disasm-verified this session @0x9e3bc-0x9e454**:
```
gun = player+0xa10; if (!gun || gun->m_iId != 0x10) return;
last = player+0xa1c;                 // the PLAYER-side last-weapon cache
if (gun->m_weaponType(0xc8) == 0) return;   // the ONLY gate: must be holding a real weapon
player+0xa1c = gun->m_weaponType;    // the COMMAND records the outgoing type
if (SelectWeapon(gun, last, true)) gun->Deploy();
```
**Divergences found (the real find):** (a) our gate `OwnedCount() <= 1` is invented — the original
allows `lastinv` only while ARMED (`m_weaponType != 0`) and doesn't count the arsenal; (b) our
`IsOwned(m_lastType)` pre-check is redundant (SelectWeapon @0x70fb8 gates owned itself); (c) the
original records the last type in the COMMAND on a PLAYER member (+0xa1c), not inside SelectWeapon
on the gun (ours: ts_gun.cpp:699 + gun member ts_gun.h:164).
**Recipe:** in `TSLastInv` (ts_gun.cpp:2926-2934) replace the gate with
`if (!pGun || pGun->m_weaponType == TSW_MELEE) return;` and drop the IsOwned check; delete
`OwnedCount` (method+decl). Defer (c) — moving m_lastType onto CBasePlayer — to the members pass
(it changes the save/prediction surface; note it in the ledger).
**Grade: YELLOW** (behavior change, but decompile-exact; playtest `lastinv` from kung-fu — original
does NOTHING there, ours previously toggled).

### Y2. `CTSGun::SlotsUsed` — demote now, stored-counter later
**Ours:** ts_gun.h:140, body ts_gun.cpp:400-424; callers `GetSpeedBySlots` (:234),
`TSPlayerFreeSlots` (:251), `TakeWeapon` capacity gate (:482).
**Original:** no recompute anywhere — TSSpace is a **stored counter** `player+0x548` (init 81 in
`TSInit` @0x82a4c), mutated inline in `CBasePlayer::PickUpWeapon` @0x8314c (cost = info+0xd0
× units, `newSpace < 0` → refuse) and the drop path; `GetSpeedBySlots` @0x66bd4 reads the counter
(cont.84 §A3, decompile inv_A3C5).
**Mechanical interim (closes the audit line, zero behavior change):** demote `SlotsUsed` (and its
per-unit-stackable logic) to a `static int TSSlotsUsed(const CTSGun*)` free function in ts_gun.cpp;
callers updated; remove from the class. Honest note: this closes the METHOD-name gap without
adopting the original mechanism — record in the ledger as interim.
**Faithful fix (bigger, do with the buy/pickup pass):** add `int m_iFreeSlots` (player+0x548,
transient name pending members_re curation), init 81 in our TSInit, mutate at
TakeWeapon/DropWeapon/MultiplyClips/flag pickup, make GetSpeedBySlots/TSPlayerFreeSlots read it.
Risk: counter drift vs our self-healing recompute; our death-scatter discards the arsenal without
symmetric give-backs today. **Grade: YELLOW** (interim GREEN-mech, faithful swap needs playtest of
encumbrance/maxspeed + buy capacity).

### Y3. `CTSGun::CanDeploy` — delete the override by fixing `GetItemInfo` to the original values
**Ours:** ts_gun.h:64 `bool CanDeploy() override { return true; }` (+ the :61-63 rationale
comment). Reached via stock prediction/deploy paths (weapons.cpp:803/:808, player.cpp:5099/:5134,
gamerules.cpp FindNextBestWeapon, cl_dll/hl/hl_weapons.cpp:189).
**Original:** CTSGun does NOT override — VTABLES.md CTSGun slot 0x124 =
`CBasePlayerWeapon::CanDeploy()`. It never refuses because of GetItemInfo:
**disasm-verified @0x75dd0 this session**: `pszAmmo1 = NULL, iMaxAmmo1 = -1, pszAmmo2 = NULL,
iMaxAmmo2 = -1, iMaxClip = -1 (WEAPON_NOCLIP), iSlot = 1, iPosition = 2, iFlags = 0, iId = 0x10,
iWeight = 1` — with `pszAmmo1 == NULL` the stock CanDeploy (weapons_shared.cpp:50) returns true
unconditionally.
**Recipe:** in `CTSGun::GetItemInfo` (ts_gun.cpp:757-772) set `p->pszAmmo1 = NULL; p->iMaxAmmo1 =
-1; p->iMaxClip = -1;` (drop the "9mm"/200/wi->iClip SLICE values); delete the CanDeploy override
(ts_gun.h:61-64). Both libs (shared file — GetItemInfo feeds the client's gmsgWeaponList too, so
this also moves the WeaponList wire TOWARD the original).
**Risks to verify (why YELLOW):** `m_iPrimaryAmmoType` becomes -1 → audit stock paths that index
`m_rgAmmo[m_iPrimaryAmmoType]` for the tsgun (ExtractAmmo/AddWeapon are null-guarded; check
`DefaultReload`, HUD ammo history); `m_iDefaultAmmo`/`iMaxClip` consumers in stock pickup code.
Playtest: deploy with empty clip, weapon pickup, HUD ammo row.

### Y4. `CTSGun::SendWeaponAnim(int)` shadow — delete; pass the body explicitly like the original
**Ours:** decl ts_gun.h:79-81, body ts_gun.cpp:2370-2375 (recomputes
`GetBodyByItems(m_weaponType, m_items[type].attach)` per call, forwards to the 2-arg base).
9 sites: ts_gun.cpp:678, :916, :1104, :1420, :1923, :1997, :2037, :2795, :2845.
**Original:** CTSGun has no such method — slot 0x174 = `CBasePlayerWeapon::SendWeaponAnim(int,int,int)`
@0x900d0 (iAnim, skiplocal, body). Callers pass the body they already hold: `Deploy` @0x71030
computes `GetBodyByItems(...)` ONCE, stores it in the item (+0x18) and `pev->body`, and later sites
pass `pev->body`; `HolsterWeapon` @0x75698 passes **0** explicitly (cont81 holster.c).
**Recipe:** delete the shadow; change the 9 sites to `SendWeaponAnim(<anim>, pev->body)` — EXCEPT
the HolsterWeapon site (:678), which becomes `SendWeaponAnim(s_drawSeq[TSW_MELEE], 0)` (original
passes 0 for the kung-fu draw). Precondition (the YELLOW part): verify `pev->body` is refreshed at
every point our shadow's per-call recompute papered over — Deploy, AddItem (attachment bought
mid-life), SwitchItems (active-toggle changes body?), Zoom/Unzoom. If any of those mutate
`m_items[type].attach` without updating `pev->body`, add the `pev->body = GetBodyByItems(...)`
store THERE (that's where the original does it). Both libs; the client's predicted viewmodel body
path (`TS_PredictedViewmodelBody`, hl_weapons.cpp:71) is independent and unaffected.
**Grade: YELLOW** (anim/body regressions are exactly the historical "attachment blink" class).

### Y5. `CTSGun::SyncActiveToItem` + `LoadActiveFromItem` — inline into Holster/Select/Deploy; the live-copy model is the flagged divergence
**Ours:** ts_gun.h:149-150; bodies ts_gun.cpp:350-360 / :363-379. Sync sites: :444 (TakeWeapon),
:639 (HolsterWeapon), :3027 (DropWeapon), :3151 (death/drop path). Load sites: :675
(HolsterWeapon→kungfu), :703 (SelectWeapon).
**Original:** neither exists (cont.84 §A4, now Deploy-confirmed). There is no park/restore because
there is **no live copy**: the original's clip/mode/hold state lives IN `items[]`
(`m_pActiveItem->…` mutated in place at fire/reload/holster time); `Deploy` loads only the
per-weapon FIREMODE from the player array `+0x49a` and clears `holdPosition`/`burstOrPhase` on the
ITEM (deploy.c); `HolsterWeapon` clears `item->fInReload`/`burstOrPhase` in place (holster.c).
**This is a state-MODEL divergence, not a method placement** — our `m_iClip`/`m_fireMode`/
`m_bHoldPosition`/`m_fInSpecialReload` shadows exist because the stock SDK's prediction reads
`m_iClip` (weapon_data). Killing the model = rewiring every fire/reload site in BOTH libs (the
whole predicted-weapon surface, memories `predicted-weapon-gotchas`/`dropin-both-directions`).
**Recipe (mechanical, closes the audit lines):** inline both bodies verbatim at their 6 call sites
(receiver-qualified at :3151); delete methods + decls; tag each inlined block
`// live-copy park/restore — NO original counterpart (items[] is mutated in place); fold-plan Y5`.
**Faithful fix:** deferred to a dedicated prediction-model pass (do NOT attempt as part of this
sweep — memory `defer-setter-without-consumer` applies: half-moving the model breaks fire/reload).
**Grade: YELLOW** (inline is safe; the tag is the point — the model stays a documented divergence).

### Y6. `CBasePlayer::TSSendScoreInfo` — delete; WAITING travels on the award byte, not ScoreInfo
**Ours:** decl player.h:588; body player.cpp:1477-1486 (broadcasts ScoreInfo with
`WRITE_SHORT(pev->playerclass)`); callers player.cpp:1501 (TSEnterSpectate), :1562
(TSSpectateThink 1 Hz reassert), :1603 (StopObserver). Plus the `pev->playerclass =
TS_SCORE_WAITING` writes (:1500, :1560-1561, :1602) and the define (:1473).
**Original:** NO helper. ScoreInfo senders are all inline (cont.84 §B4 census); the canonical
payload (AddPoints @0xc9e18, inv_B4) writes **`SHORT 0` for playerclass — a constant**, and
neither StartObserver @0x89634 nor StopObserver @0x89a14 nor the PreThink block sends ScoreInfo at
all. The scoreboard's WAITING (client case 0xb) is fed by the **award byte `player+0x74c`**
(= our `m_iAward`, player.h:525), which `SetPlayerRespawnTime` @0x65ff4 stamps to 1 (out-of-round)
/ 0xb (respawning) and TSPAward broadcasts from UpdateClientData (B1 step 14). Ours ALREADY has the
m_iAward/TSPAward pipeline (dlls/TS_gamerules.cpp:165 comment says it stamps 1/11;
ts_player.cpp:163-168 sends it).
**Recipe:** delete the helper + decl + the three calls + all `playerclass = TS_SCORE_WAITING`
writes + `pev->playerclass = 0` in StopObserver (:1602) + the TS_SCORE_WAITING define; verify
`SetPlayerRespawnTime` really stamps `m_iAward = 11` on every arm path and that TSInit/StopObserver
reset it (ts_player.cpp:698 sets 0). Also fix our own remaining inline ScoreInfo senders to the
original payload `SHORT 0` (TS_gamerulesclass.cpp:1027 AssignToTeam block writes
`pev->playerclass` — original writes team+1 with class 0; player.cpp:1910 AddPoints — check it
writes 0).
**Grade: YELLOW** (wire-facing; playtest: scoreboard shows WAITING while spectating in DM and team,
clears on spawn — against BOTH our client and the original client if possible).
**Flagged divergence:** our whole "WAITING via ScoreInfo playerclass" mechanism was an invention
layered on top of the already-reconstructed faithful path.

### Y7. `CBasePlayer::TSEnterSpectate` — multi-home split (StartObserver / InitHUD / delete-parts)
**Ours:** decl player.h:580; body player.cpp:1488-1532; callers player.cpp:1316 (PlayerDeathThink
spectate transition) and :3286 (end of Spawn, join path).
**Original:** no such method. Its pieces have four different homes (decompiles:
`inv_BC_PlayerSpawn_Observers_PreThink.c` StartObserver; `inv_C_…` InitHUD; §B2):
| our piece (player.cpp) | original home | action |
|---|---|---|
| `StartObserver(origin, v_angle)` call (:1490) | the call sites themselves (PlayerDeathThink @0x7eaec calls StartObserver directly; join = CTSGameRules::InitHUD @0x69134 calls StartObserver) | keep — callers call StartObserver directly |
| `m_iObserverLastMode = OBS_ROAMING; Observer_SetMode(OBS_ROAMING)` (:1491-1492) | **NOT original** — StartObserver ends with `Observer_SetMode(m_iObserverLastMode ?: 2)` (mode 2, chase-free, remembered-mode honored) | **DIVERGENCE — flag loudly**: we force free-roam; the original defaults to mode 2. Delete the force; make our StartObserver's trailing Observer_SetMode use `m_iObserverLastMode ? m_iObserverLastMode : 2` and verify free-look joins still work (our map-spawn spectate may LOOK different — playtest) |
| `ResetArmament()` (:1494) | `CTSGameRules::InitHUD` (inv_C line 220) — join/round-entry only | move into our `CTSGameRules::InitHUD` |
| `m_fInSpectateWait = true; m_iClientSpecSecs = -1` (:1495-1496) | no counterpart (iuser1/iuser2 are the original's markers) | keep at the two call sites for now; reconcile with iuser2 later |
| playerclass=WAITING + TSSendScoreInfo (:1500-1501) | none (award byte) | delete — Y6 |
| join respawn-gate arm (:1506-1513, respawntime clamp 5..30 → SetPlayerRespawnTime) | `CTSGameRules::InitHUD` (inv_C lines 210-221: clamp(respawntime,5,30) → SetPlayerRespawnTime(a0, secs, false)) | move into our `CTSGameRules::InitHUD` (join); the death path's gate is already armed in Killed — our conditional (`m_flTSRespawnTime <= 0 || <= time`) papered over the two homes |
| TSActiveItems{idx,0} (:1519-1523) + Spectator{idx,1} (:1526-1529) | INSIDE StartObserver @0x89634 (decompile lines 71-76, 130-137) | move both INTO our `CBasePlayer::StartObserver` (player.cpp:1388) — pure move |
| `TSSpectateThink()` initial paint (:1531) | none | delete (G7) |
Then delete TSEnterSpectate; the two callers become `StartObserver(pev->origin, pev->v_angle)` plus
the residual flag sets.
**Grade: YELLOW** (each piece is decompile-anchored, but it touches the join/death/respawn flow —
the most playtest-sensitive area in the mod; run the spectate/respawn protocol from HANDOFF after).
Note: our StartObserver already contains other original pieces (TSCash flush is MISSING — original
StartObserver's FIRST act is `+0x544 = +0x540` + TSCash MSG_ONE, decompile lines 36-42; ours
doesn't — **add while in there** and flag as a found gap. Also original clears iuser4/fuser1
slow-motion state there; ours doesn't — VERIFY ours does it elsewhere on that path).

### Y8. `CTSGameRules::PlayerKilled` — split; three of its five statements are misplaced (real finds)
**Ours:** TS_gamerulesclass.cpp:209-229 (override: base MP::PlayerKilled → DropFlag →
TSDropWeaponsOnDeath → TSDropAmmoOnDeath → TogglePlayerFromSlow → TheOneIsDead → CallCheckDeads).
**Original:** CTSGameRules has NO PlayerKilled (standalone class, §0). The virtual is the TS-patched
**`CHalfLifeMultiplay::PlayerKilled` @0x795e4** (decompile
`inv_BC_PlayerKilled_PlayerDeathThink_TSInit.c`; cont.84 §C2) — kill tiers/cash/DeathNotice/
ScoreInfo/**TheOneIsDead** (line 444)/**CallCheckDeads** (line 730). Verified this session by
caller census:
- weapon/ammo death-scatter: ALL `ThrowWeapon` call sites live in **`CBasePlayer::Killed`**
  @0x7ed5c (11 calls, 0x7f085-0x7f185) — NOT in any PlayerKilled;
- `DropFlag` on death: NOT in PlayerKilled/Killed at all — the carrier's flag drops at the
  **StartObserver** spectate transition (call @0x8975f = StartObserver+0x12b);
- `TogglePlayerFromSlow` @0x696c8 has **ZERO callers in the original binary** (dead code — our
  PlayerKilled call is pure invention).
**Recipe:**
1. Move `TheOneIsDead` block (:221-226) + `CallCheckDeads()` (:228) into our
   `CHalfLifeMultiplay::PlayerKilled` (dlls/multiplay_gamerules.cpp:675…): TheOneIsDead
   mid-function (after the frag/cash accounting, per decompile position), CallCheckDeads as the
   LAST statement.
2. Move the `TSDropWeaponsOnDeath(pVictim); TSDropAmmoOnDeath(pVictim);` pair into
   `CBasePlayer::Killed` (player.cpp:947…) at the position the scatter sits in the original
   (ts_gun.cpp:3213's own comment already cites `RE CBasePlayer::Killed @0x7ed5c`!). Longer-term,
   inline the loop bodies there (the free helpers are themselves invented free functions).
3. `DropFlag(false)`: our StartObserver (player.cpp:1390) ALREADY calls DropFlag — so the
   PlayerKilled call is a DOUBLE mechanism; delete it and verify a killed carrier's flag still
   drops (at the spectate transition, i.e. ~3 s later than today — **timing behavior change,
   original-faithful; playtest CTF**).
4. Delete the `TogglePlayerFromSlow(pVictim)` call (dead in the original; if a dead slowed player
   misbehaves in playtest, the original's coverage is StartObserver's iuser4/fuser1 clears — Y7).
5. Delete the CTSGameRules::PlayerKilled override + decl (ts_gamerules.h:115).
Side effect: CMultiplayBusters (HL leftover) would inherit the TS death behavior via MP — it never
runs in TS play; ignore (or strip busters separately, it's on the sweep's extra-cvar list anyway).
**Grade: YELLOW** (order interleave + two real timing changes; death flow is playtest-gated).

### Y9. `CTSGameRules::PlayerSpawn` — split into CHalfLifeMultiplay::PlayerSpawn + TSInit/PutInServer homes
**Ours:** TS_gamerulesclass.cpp:1039-1088 (override: SetHasSuit → team-cash seed →
game_player_equip loop → RemoveAllItems+GiveNamedItem("weapon_tsgun")+BuyArmament+TSDeployArsenal →
initial_cash seed → team auto-balance AssignToTeam(TeamWithFewestPlayers())).
**Original:** CTSGameRules has NO PlayerSpawn. The virtual is
**`CHalfLifeMultiplay::PlayerSpawn` @0x790ac** (body @0x890b8, TS_multiplay_gamerules.cpp):
`pev->weapons |= 0x80000000;` → Touch every `game_player_equip` → if none:
`GiveNamedItem("weapon_tsgun"); BuyArmament();` → spawn-cash snapshot `+0x53c = +0x540`
(cont.84 §C3; server_strips confirmed BuyArmament's ONLY caller = this @0x79122). Per-spawn TS
state reset = `CBasePlayer::TSInit` @0x82a4c; **initial_cash is given in ClientPutInServer**
(@0xacf44, inv_A3C5 line 283 `GiveCash`), NOT in PlayerSpawn — **misplacement find**; team
auto-assignment = `CHalfLifeTeamplay::SetDefaultPlayerTeam`/`ChangePlayerTeam` (TS-patched)
— R1 territory.
**Recipe (what's foldable now):**
1. Move the equip core (game_player_equip loop + the addDefault
   GiveNamedItem/BuyArmament, WITH our RemoveAllItems/TSDeployArsenal/SetHasSuit provisos tagged as
   local inventions/SDK shims) into our `CHalfLifeMultiplay::PlayerSpawn`
   (multiplay_gamerules.cpp — currently stock+partial; verify it isn't giving crowbar/9mm anymore
   after the move). Add the original's `pev->weapons |= 0x80000000` and the spawn-cash snapshot
   (`m_iSpawnCash = m_iCash` — member exists? if not, note for members pass).
2. Move `GiveCash(initial_cash)` (:1080-1081) to our `ClientPutInServer` (dlls/client.cpp) — its
   original home; gate "given once" naturally disappears (PutInServer runs once per connect).
   **Flag: today it re-seeds any player whose cash hits 0 mid-map — original doesn't. Behavior
   change, original-faithful.**
3. Team-cash seed (:1049-1050) — inert (GetTeamCash returns 0); move alongside the auto-balance
   remnant.
4. The auto-balance tail (:1083-1087, IsTeamMode/GetTeamIndex/AssignToTeam/TeamWithFewestPlayers)
   CANNOT reach its original home without R1. Interim: keep it, but as a tagged block at the END of
   CHalfLifeMultiplay::PlayerSpawn: `// INTERIM (fold-plan Y9/R1): the original assigns teams in
   CHalfLifeTeamplay::SetDefaultPlayerTeam/ChangePlayerTeam (@0xd44ac/@0xd488c)`.
5. Delete the CTSGameRules::PlayerSpawn override + decl (ts_gamerules.h:99).
**Grade: YELLOW** (spawn/equip flow; the m_fInSpectateWait no-equip branch (:1062) is coupled to
Y7's spectate flow — keep it working; playtest spawn/loadout/round-restart).

---

# RED — architectural; do NOT hand to an apply agent

### R1. The rules split (umbrella) + the CTSGameRules team quintet + AssignToTeam + IsTeamMode/IsTeamplay
**The 7 remaining CTSGameRules inventions** — `IsTeamplay`, `IsTeamMode`, `GetTeamID`,
`GetIndexedTeamName`, `RecountTeams`, `TeamWithFewestPlayers`, `AssignToTeam` — are all shadows of
the original's **two-object architecture** (§0) and cannot be folded truthfully until
`InstallGameRules` is made original-shaped (@0xb6050: install stock CHalfLifeMultiplay/Teamplay as
`g_pGameRules` (TS-patched), construct/Init the standalone `TSGameRules` object alongside — which
also retires the G1 pointer-interim into the real static object, ctor at static-init +
`Init()` at InstallGameRules).
Per-method original homes (cont.84 §C5, decompile-anchored — record verbatim in the ledger):
| ours (TS_gamerulesclass.cpp) | original home | notes |
|---|---|---|
| `IsTeamplay` :834 (+ util.cpp:1393, :728 callers via virtual) | `CHalfLifeTeamplay::IsTeamplay` @0xd5200 (stock, g_teamplay-driven); CHalfLifeMultiplay does NOT override | with the split, team maps install Teamplay → virtual resolves correctly; our override dies |
| `IsTeamMode` :822 (callers: TS_entities.cpp:410/466/481, ts_buy.cpp:120/158, ts_worldgun.cpp:584, TS_gamerulesclass.cpp:270/354/380/396/427/708/1083/1140/892) | none — the original reads the **gamemode byte (TSGameRules+0)** inline; `IsCTB` @0x67f08 exists only for mode 3 | interim (pre-R1, YELLOW if attempted): seed `m_iGameMode` with the full DecideGamePlay value (0..3, promoted to 2/3 by InitObjectives like Init @0x67d6c does: `rules+0 = g_teamplay`, `rules+1 = 0xff`), replace `IsTeamMode()` with `m_iGameMode >= 2` at all 15 sites, delete the method + the m_iTeamMode lazy cache. Requires verifying DecideGamePlay's return semantics — our current seed is a 0/1 boolean (gamerules.cpp InstallGameRules) and the objective-manager proxy compensates. This is the single highest-leverage pre-R1 item: it kills BOTH IsTeamMode and IsTeamplay-the-invention (IsTeamplay override then just returns m_iGameMode>=2 pending the split — still an override, but honest) |
| `GetTeamID` :966 | `CHalfLifeTeamplay::GetTeamID` @0xd53f4 / weak `CHalfLifeMultiplay::GetTeamID` @0xc0d9c (both stock) | dies with the split |
| `GetIndexedTeamName` :959 (callers TS_objectives.cpp:938, ts_flag.cpp:161, client.cpp:637 jointeam, maprules.cpp:437) | `CHalfLifeTeamplay::GetIndexedTeamName` @0xd54a0 (stock); the original jointeam (@0x9e4b0) calls it via g_pGameRules vt+0xd4 | NOTE: the original's TS code paths that need a team NAME read the TSGameRules 4-slot table directly (name @+0x28+i*0x8c) — after R1, TS_objectives/ts_flag read TSGameRules's table, jointeam goes via the Teamplay virtual |
| `RecountTeams` :927 (callers :419 RestartRound, :856 SetTeamList) | `CHalfLifeTeamplay::RecountTeams(bool)` @0xd5608 (stock) — RestartRound @0x67920 calls it ON g_pGameRules (inv_C line 388); and **SetTeamList @0x780dc tokenizes straight into the 4 slots itself** (inv_A3C5 decompile — no parse helper, no m_szTeamList buffer, "created team %s" ALERT, The-ONE insertion, cap 2 in theOne mode else 4) | pre-R1 partial (YELLOW): rewrite our `SetTeamList` decompile-exact (inline tokenize, drop m_szTeamList + the :856 RecountTeams call), leaving :419's call as the tagged Teamplay-stand-in; full fix needs R1 |
| `TeamWithFewestPlayers` :975 | `CHalfLifeTeamplay::TeamWithFewestPlayers` @0xd5510 (stock) | dies with the split |
| `AssignToTeam` :1005 (callers client.cpp:640 jointeam, :1087 PlayerSpawn) | TS-patched `CHalfLifeTeamplay::SetDefaultPlayerTeam` @0xd44ac + `ChangePlayerTeam` @0xd488c (which does the model infokey via `CTSGameRules::GetPlayerModel` @0x69a08, TeamInfo/ScoreInfo, then StartObserver + SetPlayerRespawnTime(player, 5, true)) | our jointeam path should become GetIndexedTeamName → **ChangePlayerTeam** (matching the original @0x9e4b0 dispatch), which our tree's teamplay_gamerules.cpp already has in stock form — the TS patches to it are the RE work here |
**Why RED:** every fold above either strands logic on a class our installed rules object doesn't
derive from, or requires the InstallGameRules redesign + the stock-object gutting
(server_strips TASK 4 found the original's `new`ed shells are TINY — CHalfLifeMultiplay 0xc bytes —
i.e. the original also GUTTED the stock classes' state; reproducing that faithfully is a real
design task, not an edit).
**Ledger carry:** R1 = "rules split: install stock MP/Teamplay + standalone TSGameRules object;
retires 7 invented methods + the G1 pointer interim".

### R2. The 6 invented CLIENT globals — re-verified, none mechanically fixable (one line each)
(server_strips TASK 4 verdicts re-checked against the current tree; client.dll has no symbols —
AUDIT's UNVERIFIABLE caveat applies)
- `g_TSGun` (cl_dll/hl/hl_weapons.cpp:55) — the predicted CTSGun instance; stock-SDK file-scope
  weapon-object precedent; original client counterpart unnamed/unknowable. **Accept/defer.**
- `g_tsActive` (cl_dll/ts_weapon_predict.cpp:25; consumers ev_ts.cpp:849/:970) — event-handler
  active-weapon cache; the original fire handler's state model (client FUN_ addresses) still
  un-RE'd. **Defer until the ev_* handler RE.**
- `g_tsLocalFov` (cl_dll/entity.cpp:74 → ts_scope.cpp:22) — clientdata→scope fov conduit; no
  verified original member to re-home onto (moving it invents a member instead of a global).
  **Defer.**
- `g_tsPlayerGunAtt` + `g_tsPlayerGunAttTime` (cl_dll/StudioModelRenderer.cpp:24/25; staleness
  check ts_attachments.cpp:143) — per-entity attachment anchor cache; original likely reads
  `cl_entity_t` attachments at use-time (needs client RE). **Defer.**
- `g_tsPool` (cl_dll/ts_particles.cpp:54, already file-`static` — arguably shouldn't count as an
  audit-visible global at all) — our particle pool; the original TSPSys handler is itself still
  MISSING (sweep §A1). **Restructure WHEN TSPSys is RE'd.**
**Ledger carry:** all 6 = known-unverifiable-by-design pending client-side RE; none block anything.

---

## Execution order for the apply agent (GREENs only)
1. G1 (TSGameRules global) — biggest sed first, everything else rebases on it.
2. G3, G4 (gamerules inlines — same file).
3. G2 (player UCD inline).
4. G6, G7 (client.cpp/player.cpp inlines).
5. G5 (ts_gun inline, both libs).
Then: `scripts/build.sh amd64 all` (both libs must link), `scripts/check-source-lists.py` (no file
adds/removes expected — pure inlines), re-run `scripts/coverage/audit.py` → INVENTED methods should
drop 23 → 12 (remaining: Y1-Y9 items 8 + the R1 quintet — exact count depends on which YELLOWs ride
along) and MISSING globals 1 → 0. No playtest strictly required for GREENs (pure moves), but the
next interactive round should re-run the spectate/respawn protocol before any YELLOW lands.

## The "real finds" (logic divergences unearthed by this plan — ledger-worthy independent of folds)
1. **lastinv gate** (Y1): original = "must be armed" (`m_weaponType != 0`), last-type recorded by
   the COMMAND on the PLAYER (+0xa1c); ours = OwnedCount>1 + gun-side m_lastType set in SelectWeapon.
2. **GetItemInfo wire** (Y3): original tsgun = NO ammo types, iMaxClip -1; ours ships "9mm"/200/
   wi->iClip into gmsgWeaponList.
3. **WAITING mechanism** (Y6): original = award byte 0xb via SetPlayerRespawnTime→TSPAward;
   ScoreInfo playerclass is a CONSTANT 0. Our ScoreInfo-playerclass WAITING is an invented parallel
   wire.
4. **Death-drop homes** (Y8): scatter = CBasePlayer::Killed; flag drop = StartObserver (spectate
   transition, NOT kill time); TogglePlayerFromSlow = dead code in the original.
5. **initial_cash home** (Y9): ClientPutInServer @0xacf44, not PlayerSpawn — ours also re-seeds on
   any 0-cash respawn.
6. **Observer default mode** (Y7): original StartObserver → mode 2 (remembered), ours forces
   OBS_ROAMING; original StartObserver also flushes TSCash + clears iuser4/fuser1 slow state — ours
   doesn't (gap).
7. **SendRoundTime gate** (G3): original sends clamped-0 for an expired clock + honours the
   round-timer-enabled flag; UCD also re-sends it in the InitHUD block (ours doesn't).
8. **SetTeamList shape** (R1): original tokenizes into the slots directly, "created team %s",
   The-ONE inserted as team 0 with cap 2; our m_szTeamList+RecountTeams parse is invented plumbing.
9. **Stock Health/Battery from UCD** (G2 follow-up): original TS UCD appears to send ONLY the TS
   messages — VERIFY, then strip ours.
