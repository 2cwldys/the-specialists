# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-29 (cont. 75) — source-parity restructure cont.: the CTF cluster, bottom-up (1:1)

Branch `restructure-source-parity` (continues cont.74; GPG signing OFF). Same loop. This session
reconstructed the **whole CTF cluster** to 1:1 parity from a dossier subagent's RE
(`scratchpad/dossier-CTF.md`). Commit `a4254ec`. Builds amd64 server+client clean (glibc 2.14).

- **Key structural finding (the carry model):** there is NO "flag follows the carrier" entity. The
  flag is a 28-byte **`s_flag_status` value struct** copied byte-for-byte at every hop (briefcase→
  flag→player→flag…); a `CTSFlag` entity exists ONLY while the flag is loose. On pickup TakeFlag
  copies the struct onto the carrier (CBasePlayer+0x7d0) and **Destroys** the flag; on drop a fresh
  loose flag is `CreateFlag`d. Our old `m_hCarrier`/`CarryThink`/`AttachTo` + the free
  `TSCreateFlag/TSDropFlag/TSPlayerHasFlag/TSReturnCarriedFlag` helpers were all INVENTED — deleted.
- **s_flag_status (player.h):** active byte / contents string_t / flagID 1..15 / team 0-based /
  lastCarrier entindex / **EHANDLE hBriefcase** (the respawn target rides INSIDE the struct).
- **CBasePlayer::TakeFlag/DropFlag + free HaveFlag** (ts_player.cpp / ts_gamerules.cpp): pickup =
  copy status + the **20 slot-weight debit** (folded into TSSlotBaseSpeed/TSPlayerFreeSlots in
  ts_gun.cpp → carrying the flag slows you, the `members-globals` rule: the "+0x548 −0x14" wasn't a
  dead member, it's the encumbrance). DropFlag(false)=spawn loose flag (toss from eye, random
  avelocity, dead→random dir 60..160), DropFlag(true)=silent capture clear. gmsgTSHaveFlag (0/1/2).
- **CTSBriefcase 6/6 0-inv** (ts_briefcase.h/.cpp): the flag REGISTRY owner — static `s_nextFlagID`
  cycling 1..15 (ctor), template status w/ EHANDLE=self (Spawn), FlagFall seeds the flag +
  gmsgTSHaveFlag type 0, **CaptureTouch** = +10 frags + gmsgScoreInfo/TeamScore/TSVoiceMessage +
  respawn the captured flag at ITS home base via the carried status' EHANDLE. `contents` is a
  **char[32]** (strncpy), not string_t. (FILE MISMATCH remains: orig groups it in TS_objectives.cpp.)
- **CTSFlag 15/15 0-inv** (ts_flag.h/.cpp) **: CTSPhysicObject** — mirrors the CTSWorldGun settle/
  bounce machine (Spawn/BounceTouch/Fall/OnGround/WaitThink/WaitTouch). CreateFlag(owner,status)/
  RespawnFlag (asks the briefcase to re-drop, never walks home)/Destroy/PlayerTakeFlag(=Use=WaitTouch:
  enemy→pickup, own dropped→return +5)/CanInteract/ObjectCaps(0x10)/CanUsedTroughWalls(0). Thrown-flag
  momentum TakeDamage in Fall. Extracted to a HEADER (fixes the audit .h-only blind spot; lets
  briefcase/gamerules derive it).
- **Wiring:** DropFlag(false) from PlayerKilled / StartObserver / ClientDisconnect / TSDropWeapon;
  RestartRound clears carried flags (DropFlag(true)) + RespawnEntities now calls CTSFlag::RespawnFlag
  (was a VERIFY `Spawn()`). Removed the obsolete `tsspawnflag` console stand-in.
- **Metrics:** invented methods **117 → 110**, 0 missing / 0 invented classes, file mismatches 22.
- **⚠ SCORING corrects the early handoff note:** capture = **+10** frags (CaptureTouch); returning
  your own dropped flag = **+5** (PlayerTakeFlag). VERIFY slices: alive-drop toss speed (200), the
  exact CaptureTouch team-score field (used m_teamScore), s_flag_status @+0x254 base-spawn field.
- **UNPLAYTESTED** — the whole CTF flow needs a live pass before merge (with the rest of cont.74/75).

---

## 2026-06-28 (cont. 74) — source-parity restructure cont.: drifted classes' MISSING methods (+drop keepalives)

Branch `restructure-source-parity` (continues cont.73; GPG signing OFF). Same loop: `audit.py <Class>` ->
subagent dossier (`re.sh`, single-writer) -> reconcile our working logic into the original's exact method/
member layout -> `build.sh amd64 all` -> `audit.py <Class>` (0/0) -> commit. **User sharpened the directive:
ANY missing/extra member or global = hidden logic inconsistency -> re-audit + Ghidra, never paper over** (saved
as memory `members-globals-signal-hidden-logic`). Each class drops a keepalive by building real wiring.

- **CTSWorldGun cluster 4/13+3/7 -> 13/13+7/7** (`0df448f`): merged ts_ammopack.cpp INTO ts_worldgun.cpp
  (+new ts_worldgun.h) = TS_worldgun.cpp file parity (mismatch -1). **CTSWorldGun now derives CTSPhysicObject**
  (dossier confirmed; hierarchy.tsv MISSES it). Replaced invented FallThink with the real method split:
  CreateWeapon (static factory carrying the full s_weapon_status = our **TSItem**, the 0x38-byte slot) / Spawn
  (MOVETYPE_BOUNCE, gravity 0.7/friction 0.8) / BounceTouch (drop sound + 0.7 restitution) / Fall (settle +
  swept-trace impact damage — a thrown gun hurts what it flies into) / WaitThink / OnGround / Use (cooldown-gated)
  / GetWeapon (restores the full item to the arsenal) / WaitTouch / ShouldCollide (collide only vs WorldGun/knife,
  pass through players) / ObjectCaps / CanUsedTroughWalls. classname **"WorldGun"** (NOT ts_worldgun — fixed
  ts_sellzone match). Global **ShouldCollide hook wired into gNewDLLFunctions (pfnShouldCollide)**. CTSAmmoPack:
  carries **ARMOR** (pev->armorvalue via CBasePlayer::TakeArmor, removal keyed on armor==0); ammo via CTSGun::
  AddAmmo; classname "AmmoPack". Drop helpers in ts_gun.cpp guarded `#ifndef CLIENT_DLL`; carry the full TSItem.
  CTSPhysicObject now kept by CTSWorldGun (grenade keepalive slimmed). **CTSPhysicObject's curated trio
  (m_flExpire@0x88/m_pOwner@0x8c/m_iType@0xa0) is a curator MIS-HOIST** — they're grenade-subclass members that
  COLLIDE with CTSWorldGun's 0x88/0x8c; do NOT add to the base (verified via decompile, not the label).
- **CM61Grenade 0/6 -> 6/6** (`2dffd07`): was structurally off (derived stock CGrenade w/ invented Throw()+
  M61WaitThink()); real chain is **CM61Grenade : CTSGrenade : CTSPhysicObject**. Real 6 methods: CreateGrenade
  (static; propagates the thrower's slow-mo via fuser1=m_flSlowMotionGo) / Spawn / WaitThink (fuse += GiveSlowMul
  ()*0.1 = our existing max(fuser1,0.01)*0.1; 64u swept break-through; CTSPhysicObject::WaitThink upkeep; detonate)
  / BounceTouch (owner-skip -> CTSGrenade::BounceTouch) / Use (+USE re-throw at 625 u/s) / ObjectCaps (0x10).
  Removed invented avelocity tumble; added the missing CTSPhysicObject::WaitThink call. **DROPPED the
  ts_grenades_keep_physic keepalive** (CTSGrenade now kept via CM61Grenade inheritance). TSThrowM61 lost its
  damage param (orig carries none); blast dmg/radius fixed in WaitThink (VERIFY: recovered literals 300/1024 vs
  old tuned 400). Client stub updated; obsolete TSCreateWorldGun/TSCreateAmmoPack stubs removed.

- **CBreakable 17/22 -> 22/22** (`d63559d`): the TS breakable-RESPAWN system, all additive in tails so the
  playtest-confirmed glass/debris (gmsgTSCreateDebris) Die path is untouched. **Killed** override (records attacker
  global -> Die; TakeDamage now calls Killed ALONE — was Killed+Die, would double-Die). **BreakableRespawn(bool)**
  (@0x64e84): box-clear retry (UTIL_EntitiesInBox FL_CLIENT|FL_MONSTER) -> clear: restore deadflag/health/effects +
  Spawn (+fire respawntarget on SF 8); blocked: re-arm helper +5s; disabled: SUB_Remove. Spawn creates+cross-links
  the **CInfoBreakable** helper when respawntime>=0; Die's tail hides it (EF_NODRAW) + arms helper.RespawnThink at
  time+respawntime (0=>30s). New members m_flRespawnHealth/m_hRespawner/m_flRespawnTime/m_szRespawnTarget (not saved,
  matching orig); keyvalues respawntime/respawntarget. **CInfoBreakable: added LINK_ENTITY_TO_CLASS(info_breakable)**
  (Create now resolves), RespawnThink fixed to just delegate to BreakableRespawn(false) (was wrongly resetting+Spawn+
  removing itself — the helper PERSISTS). **DROPPED ts_func_break_keep keepalive.** Moved stock Explodable/Explosion
  Magnitude/ExplosionSetMagnitude out-of-line (orig has real symbols) so audit sees them.

- **CTSGun 28/47 -> 30/47** (`a6a2669`,`<chunk3a>`): started the big name-map restructure of the central
  (client-predicted, heavily-playtested) weapon class in SAFE chunks, driven by the name-map dossier
  (`scratchpad/dossier-CTSGun-namemap.md` — the OUR->REAL map + groups A/B/C/D + chunk order). Chunk 1: pure
  rename CloseCombatBlock->CCombatBlock. Chunk 2: added the 2 missing NAMED members m_pActiveItem(+0xcc)/
  m_pActiveInfo(+0xd0) (kept in sync by UpdateActivePointers() at every m_weaponType assignment; the orig's
  `m_pActiveItem!=NULL` guard == our `m_weaponType!=TSW_MELEE`). Chunk 3a: ShouldWeaponIdle override (return true).
  **⚠ NEXT chunks are BLOCKED (assessed via the captured decompiles g1/g2/g3.c — NOT safe-additive):**
  Reset@0x805e0 needs a call site in CTSGameRules round/spawn-reset (UNBUILT) else it DCE's = dead/still-missing;
  CCombatIdle@0x85e54 plays `get_weapon_info(0)->[wi+0x252]` (the active-anim-block idle index) which our modern
  s_weapon_info doesn't carry — needs the weapon-table data extraction to avoid GUESSING the seq; EmptySound@0x71be4
  plays a predicted client event (field_0x968) — needs the event registered + client handler; HolsterWeapon@0x85698
  + chunk-4 DropType->DropWeapon/SelectWeaponType->SelectWeapon move Deploy out to EVERY caller of the playtested
  weapon-switch flow — a reshape that wants a playtest to verify. So CTSGun's tail needs groundwork first:
  (a) the weapon-table anim-block extraction, (b) CTSGameRules' reset path, and/or (c) a playtest budget — do those
  BEFORE resuming, rather than guess/dead-method. Chunk 6 (melee split KnifeFire/KatanaFire/KatanaThrow + the
  SecondaryAttack keystone + StopReload/ReloadWeapon, then dissolve invented ItemPostFrame) is the deepest, last.
  Group D KEEP (legit, not drift): CanDeploy/GetWeaponData/SetWeaponData (stock virtuals), OwnedCount/SlotsUsed/
  Sync+LoadActiveFromItem/RegenBlockBar/SwitchBarrel/SendWeaponStatus/UpdateActivePointers (model glue).

- **CTSPowerUp 11/11 0-invented** (`3f81e4a`) + **CTSSlowMotionPoint 4/5->5/5** + **CTSBriefcase 6/6 0-invented**
  (`5e56977`): a batch of clean small-class reconciles. **Reusable pattern found — INVENTED `ObjectCaps`:** several
  TS classes override ObjectCaps where the ORIGINAL has none (uses base) — confirm via decompile (the method count
  excludes ObjectCaps), then REMOVE ours (the FCAP_ACROSS_TRANSITION/DONT_SAVE difference is inert for runtime-created
  or MP-only entities). Did CTSPowerUp (return ~ACROSS_TRANSITION), CTSSlowMotionPoint, CTSBriefcase (return
  FCAP_DONT_SAVE). Also: CTSSlowMotionPoint Create->CreateSlowMotionPoint (name match); CTSPowerUp members
  m_iType/m_iRespawnTime/m_flMult -> poweruptype/pwupduration/pwupmult (the orig FGD/save names, offsets already matched).
  ⚠ **TOOLING BLIND SPOT found:** `audit.py our_members()` only scans **.h files** — so for classes declared in a .cpp
  (CTSPowerUp/CTSFlag/CTSSlowMotionPoint/CTSBriefcase/…) it reports "0 ours / MISSING named: <all>" REGARDLESS of our
  member names. Those member "gaps" are FALSE — verify by reading the .cpp. (Fix would be teaching our_members to
  brace-track + scan .cpp, but that risks misparsing .cpp locals — left as a tooling follow-up.)
- **CTSDMHill 3/7->7/7** (`179777b`): reshaped our think-poll (invented HillThink+Award) to the original's
  TOUCH-driven king-of-the-hill — WaitTouch (holder tracking: first toucher claims; same-team keeps; enemy resets;
  payout past the award time, team mode awards every teammate) + WaitThink (reschedule) + Save/Restore + members
  renamed nextcheck/fragseverycheck(kv fragamt)/fragseverycheckteam(kv fragamtteam). ObjectCaps->0.
- **CTSObjective 1/7->7/7** (`b878bb9`): the objectives-cluster BASE. CanInteract(CBasePlayer*)/(int) + Freeze
  (out-of-line) + IsObjective()->true + EnableObjective(bool) + Save/Restore (1-field=attacker_team). Freeze's
  CTSGameRules::ShowObjectiveOnHud stays SLICE (unbuilt). **⚠ STRUCTURAL finding:** the original objectives cluster
  (CTSBomb/CTSHack/CTSTeamEscape) DERIVES CTSObjective; OURS has each CBaseEntity-direct with DUPLICATED attacker_team/
  CanInteract. Cluster reconcile = make them derive CTSObjective + build CTSGameRules' objective methods they call —
  a multi-class job interdependent with CTSGameRules 12/39. Decompiles captured: scratchpad/obj.c, hill.c.
- **Objectives-cluster restructure STARTED** (`c5fe12d`,`419422b`): **extracted CTSObjective into dlls/ts_objectives.h**
  so the per-objective classes in their own .cpp can DERIVE it (also makes CTSObjective's members header-visible →
  audit no longer false-flags them). **CTSBomb 9/11->11/11**: derives CTSObjective (drops dup m_attackerTeam/CanInteract;
  m_bEnabled->inherited m_bActive); +Save/Restore (2-field=defusesec/cancutwire); members renamed
  defusetime->defusesec/cutwires->cancutwire; dissolved invented Explode inline into WaitThink. **CTSTeamEscape
  5/9->9/9**: derives CTSObjective; +Save/Restore (1-field=escape_percentage)+Precache(empty)+ObjectCaps(0x10);
  dissolved invented AttackerTeamSize inline; completion uses Freeze(true). **PATTERN for the cluster:** derive
  CTSObjective, drop the duplicated attacker_team/CanInteract, map the class's "enabled" flag to the inherited
  m_bActive(+0x64), add Save/Restore via IMPLEMENT_SAVERESTORE(<Class>, CTSObjective), rename members to the
  FGD/save names, dissolve invented helpers the original inlines. **CTSHack 4/9->9/9 DONE (448d27d): Use-driven held-USE hack (m_flHackProgress from hacksec) + WaitThink objective deadline (m_flObjTimer seeded from m_flRoundEnd) + EnableObjective/ObjectCaps/Save/Restore; dissolved invented HackTouch. ALL 4 core cluster classes (CTSObjective/CTSBomb/CTSTeamEscape/CTSHack) DONE.
  **CLUSTER BOUNDARY:** the WIRING classes are BLOCKED on CTSGameRules' unbuilt objective methods — CTSObjectivePointer::Use
  (only-missing) finds the targeted objective, EnableObjective(true)s it, then needs CTSGameRules::AddObjectiveInList +
  SendObjectiveToHud(idx,player) per client (both unbuilt). CTSObjectiveManager (Save/Restore + targets[8]/messages[4]/
  end_round_target) + CTSWinGiver (EnableObjective/IsObjective/Save/Restore + cash_for_winner/loser/numberofcall/winnerteam)
  remain; their HUD/list flow also needs the CTSGameRules objective subsystem (AddObjectiveInList/SendObjectiveToHud/
  SendObjectiveList/ShowObjectiveOnHud/SetObjective/InitObjectives + TeamWin). DO THAT SUBSYSTEM NEXT (part of CTSGameRules
  12/39), then Pointer::Use/Manager/WinGiver wire up. Decompile: scratchpad/ptruse.c.
- **CTSGameRules objective subsystem DONE (68fba35): 12/39 -> 20/39.** Built the 8 methods + 2 gmsgs (dossier
  scratchpad/dossier-gr-objectives.md): HUD list (m_objectiveList[32] of {CTSObjective*+per-team label string-idx}+count) —
  ClearObjectiveList/FindIndexByObjective/AddObjectiveInList/SendObjectiveToHud('Objective' -1: coord xyz+byte idx+string
  label, MSG_ONE)/SendObjectiveList/ShowObjectiveOnHud('ShowObj' 2: byte idx+byte show, MSG_ALL); manager array
  (m_managers[32]) — InitObjectives(scan ts_objective_manager + disable objectives by classname)/SetObjective(random manager
  +Reset). Extracted CTSObjectiveManager into ts_objectives.h. **UN-SLICED CTSObjective::Freeze -> ShowObjectiveOnHud** (the
  cluster's freeze/complete now updates the HUD). ⚠ **TWO BLOCKERS for the rest of the wiring:** (1) the original's
  IsObjective is a CBaseEntity virtual (every entity answers it) — OURS is CTSObjective-only, so you can't call
  ->IsObjective() on an arbitrary found entity (InitObjectives works around it by enumerating objective CLASSNAMES;
  CTSObjectivePointer::Use finds by targetname so it can't). (2) CTSObjectivePointer::Use reads the objective via
  *(CTSObjective**)(found+0x7c) — UNCERTAIN field. To finish Pointer::Use/Manager/WinGiver: either add IsObjective() to
  CBaseEntity (faithful — orig has it at vtbl 0x1c; verify audit doesn't flag invented) or classname-gate, AND RE the +0x7c
  ref (live trace/decompile). Remaining CTSGameRules gap (19): voting/team-cash/round-restart/TheOne subsystems.
- **OBJECTIVES CLUSTER COMPLETE (5609490, 88d29fa).** Added IsObjective()/EnableObjective() as CBaseEntity virtuals
  (the original has them — IsObjective__11CBaseEntity/EnableObjective__11CBaseEntityb, audit listed both MISSING on
  CBaseEntity; default false/no-op, CTSObjective-derived override) -> CBaseEntity 70->72/85. Solved Pointer::Use's
  *(CTSObjective**)(found+0x7c) = the found edict's pvPrivateData = CBaseEntity::Instance (the found ent IS the
  objective). CTSObjectivePointer 2/3->3/3 (Use: per targeted IsObjective() ent -> EnableObjective(true) +
  AddObjectiveInList(obj, the pointer's 4 per-team labels) + SendObjectiveToHud to all). InitObjectives now does the
  faithful all-entity IsObjective() sweep. **CTSObjectiveManager 5/5** (+Save/Restore 13-field=8 targets+4 messages+
  end_round_target; members renamed m_objTargets->targets/m_messages->messages/m_targetTimeLeft->end_round_target).
  **CTSWinGiver 7/7** (+Save/Restore 4-field=winnerteam/cash_for_winner/cash_for_loser/numberofcall + IsObjective()->1
  + EnableObjective override [orig ignores arg, just resets]; members renamed). Whole cluster (CTSObjective base +
  Bomb/TeamEscape/Hack/Trigger/WinGiver/ScoreGiver derive it w/ Save/Restore; Manager/Pointer wire the HUD; CTSGameRules
  8 methods + Objective/ShowObj gmsgs) reconstructed end-to-end. ⚠ CLIENT HUD handler for Objective/ShowObj = cl_dll track.
  Still OPEN (not blocking): the array-element audit-naming artifact on Manager targets[]/messages[] (DEFINE_ARRAY vs
  per-element — data saves fine).
- **CTSGameRules round-restart subsystem DONE (0a7832c): 20/39 -> 25/39** (dossier scratchpad/dossier-roundrestart.md).
  Reshaped our static TSRestartRound into the original's methods + WIRED InitObjectives/SetObjective into the round
  lifecycle (retiring the cluster's Spawn-self-arm SLICE intent): **Init()** (+a CTSGameRules ctor calling it: InitTeams +
  InitObjectives->SetObjective + m_flRoundRestart=time+5 + round-timer-enabled); **RestartRound()** (re-arm timer inline
  [roundtime clamp 30..180] + disarm restart + RoundTime msg + InitObjectives->SetObjective + team banner + respawn players
  + RecountTeams + RespawnEntities(true); now the member called from Think); **RespawnEntities(bool)** (info_breakable->
  RespawnThink + ts_flag always; ts_groundweapon->Fall team/lms; ts_powerup lms — VERIFY: powerup/flag use Spawn() vs orig
  Fall/RespawnFlag since those classes are .cpp-local); **ResetRestartRound(int)** (round-end arm-1s-restart, team/lms-gated,
  +"Restarting Round", depth>1 wipes scores — TeamWin now calls it instead of an inline +3s); **CountPlayers()**. Made
  ts_groundweapon.h self-contained. SLICEs: RestartRound's loadout cash-refund + force-class-select-observer (we respawn
  directly); ResetRestartRound's per-team cash reset + TeamScore broadcast.
- **CTSGameRules team-cash/list subsystem DONE (6c9cbe2): 25/39 -> 31/39.** Added GetTeamsNum (active-team count),
  SetTeamList (parse ';'-list -> roster + RecountTeams), SetTeamModelList (parse '|'-list + the per-slot (int,model)
  overload), GetTeamCash (player's team-pool cash; the cash economy is hard-OFF so returns 0), and InitTeamScore/
  UpdateTeamScores (**NO-OP stubs in the original**). Added m_teamCash[]. InitTeams now drives SetTeamList/SetTeamModelList;
  the rest wired to faithful callers (GetTeamsNum->InitHUD, GetTeamCash->PlayerSpawn[inert@0], InitTeamScore->Init,
  UpdateTeamScores->ResetRestartRound) so --gc-sections keeps them.
- **CTSGameRules COMPLETE — 39/39 methods, 0 missing (b48ecbf/92256db/cc7059b).** Finished the last 3 subsystems:
  **TheOne juggernaut** (IsCTB[ts_briefcase cache]/SelectRandomTheOne[throttled crown]/SetPlayerTheOne[buff: health 150+
  regen+kungfu+low-grav+maxspeed 330]/TheOneIsDead[killer inherits]; +CBasePlayer m_bTheOne; wired Think+PlayerKilled);
  **GetPlayerModel**(random team model, wired AssignToTeam) + **TogglePlayerFromSlow**(GoSlow(1,0), wired PlayerKilled);
  **voting** (PlayerVote['vote reset|stats|map <name>', IS_MAP_VALID] + CheckVotes[10s tally of map/ff/teamplay votes ->
  majority=CalcMiniminVotants=max(votants/2,1) -> mp_friendlyfire/mp_teamplay/next-map]; wired ClientCommand 'vote' + Think).
  CTSGameRules went **12/39 -> 39/39 this session** via 5 subsystems (objective, round-restart, team-cash/list, TheOne,
  voting). The 12 "invented" methods are legit: stock virtual overrides (PlayerKilled/PlayerSpawn) + TS team helpers
  (AssignToTeam/IsTeamMode/GetIndexedTeamName/GetTeamID/RecountTeams/etc.) under our reconstruction names. SLICEs noted in
  commits (cash refund/observer, ff/teamplay vote commands, next-map application, the_one powerup value). UNPLAYTESTED.
  derives CTSObjective; needs +Save/Restore(hacksec)+EnableObjective+ObjectCaps(0x10) and to dissolve our invented
  HackTouch into the original's **Use** (a 688-byte hack-interaction method) + the WaitThink countdown-to-Freeze model
  (EnableObjective seeds field_0x6c timer; WaitThink decrements each sec -> Freeze(true) at 0). Decompile: scratchpad/hack.c.

audit headline: invented methods **131->117**, file mismatches **23->22**, 0 missing/0 invented classes; **3
keepalives dropped/slimmed** (CTSPhysicObject now kept by CTSWorldGun, CTSGrenade by CM61Grenade, CInfoBreakable
wired+LINK'd). Remaining keepalive: the compiled-but-unlinked pistols (CTSPistolWeapon/Ammo — the ORIGINAL keeps
them too, legit). All build amd64 server+client (glibc 2.14). **⚠ ALL UNPLAYTESTED** — bounce physics, swept impact
damage, armor drop, grenade slow-mo + M61 blast literals, breakable respawn. NEXT: the **CTF cluster** (deferred —
bottom-up: s_flag_status -> CBasePlayer::TakeFlag/DropFlag -> CTSBriefcase -> CTSFlag 4/15; dossier
`scratchpad/dossier-CTSFlag.md`), CTSGameRules 12/39 + the objectives cluster, CTSGun's remaining missing methods,
the ~126 invented methods to review.

## 2026-06-28 (cont. 73) — code-restructure to source parity (audit.py-driven): 7 classes -> 1:1

Branch `restructure-source-parity`. Per the user's directive — ONE CLASS AT A TIME, make methods + variables
match the original 1:1 (nothing missing, nothing added), using subagents that write `.md` dossiers to scratchpad
(keep raw decompiles out of context). Loop: `re.sh <Class>` decompile -> dossier -> reconcile our WORKING logic into
the original's exact method/member layout -> build amd64 -> `audit.py <Class>` -> commit. Driver = `scripts/coverage/audit.py`.
GPG signing off for the session (`git config --local commit.gpgsign false`).

5 commits `69c21ab`..`8f73a46`. audit headline: invented methods **145->132**, file mismatches **25->22**,
0 invented classes (unchanged). Each builds amd64 (glibc 2.14); the BEHAVIOUR-sensitive ones need a playtest pass.

- **CTSAnimation 13/13** (`69c21ab`): split our monolithic `Animate()` + 4 collapsed helpers into the original's 13
  methods (Init/UpdateTimers/Animate 4-phase / Enable/Disable/BulletDance / BlendAnimationsForSlide/ByDir/Jump /
  SetAnim / HandleCloseCombat / IsInCloseCombat / GaitCloseCombat). `SetBodySequence`+`SetGaitSequence` -> the flag-driven
  `SetAnim` (orig flags 0x92/0x94/0x14/0x194/0x12). `GetMoveDirection` -> the free `GetDirection` (orig
  `GetDirection__FG6VectorT0ff`). `PlayKungFu` -> `HandleCloseCombat(player,code)`; ts_gun's 3 PlayKungFu(name) sites ->
  `Animate(player,closeCombatCode)`. `m_bKungFu` bool REMOVED — kung-fu keys off the EMPTY grip (melee Deploy now passes
  ""; also fixes the phantom `draw_onehanded` pistol-draw on fists). Added bullet-dance state + GiveSlowMul timer scaling.
- **CTSStunt 25/25** (`000f2a3`): the 5 INVENTED helpers leave the class surface — `GetDirection` = the shared free fn;
  `TimeScale`/`SendNextStunt`/`MotionBlur`/`ResetStuntState` -> file-local statics (the original inlines them; like the
  existing TSWallTrace). `SlotBonus`/`NextDive` out-of-line. `CloseAttack` (the JUMP-KICK <- CBasePlayer::Jump)
  reconstructed AS a CTSStunt method — relocated from the free fn `TSTryJumpKick` (removed; invented-global cleared;
  `m_flNextJumpKick` member removed, uses `m_flStunCooldown`=CTSStunt+8). `CheckForBreakables` (<- PreThink) added: the
  non-melee run/dive-into sweep shatters world breakables (players excluded; the close-combat Victim*Hold path needs unbuilt
  state -> DEFERRED+VERIFY). `TimeScale` fixed to `max(fuser1,0.01)`. A caller-finder subagent (`re.sh refs`/`vrefs`)
  confirmed: CheckForBreakables<-PreThink, CloseAttack<-Jump; the +0xb4 damage-virtual + GetDirection's networked +0x2a0
  store are VERIFY-flagged.
- **CTSDeadCorpse 5/5 + file rename** (`683c123`): FadeThink->WaitThink; add empty Precache + no-op TraceAttack;
  ObjectCaps -> FCAP_DONT_SAVE (0x80000000). `ts_corpse.cpp` -> **`ts_deadcorpse.cpp`** (LINK name `ts_corpse` unchanged).
- **CTSShrapnel 6/6 + file rename** (`feb4e02`): FadeThink->WaitThink. `ts_grenade.cpp` -> **`ts_grenades.cpp`** (also
  clears CM61Grenade's file-mismatch — shares the file; CM61Grenade methods still drift, separate pass).
- **CTSPlayerAmount 4/4** (`8f73a46`): event-driven `ClientConnect`/`ClientDisconnect` (per-entity counter, fire on
  ==clients) replace the invented `CountThink` poll + `CountConnectedPlayers`; derive CBaseDelay; wired from the engine
  ClientConnect/Disconnect hooks in client.cpp (new ts_playeramount.h). (File still ts_playeramount.cpp vs the original's
  grouped TS_entities.cpp — grouping divergence, per HANDOFF.)
- **CTSTrigger 5/5 + CTSScoreGiver 3/3** (`887c58f`): CTSTrigger += ObjectCaps()=0 + WaitThink (reschedule, wired in
  Spawn). CTSScoreGiver += explicit KeyValue (delegates to CTSWinGiver) + Spawn (hidden relay). NB the objectives cluster
  (CTSObjective/CTSObjectiveManager/CTSObjectivePointer/CTSHack/CTSWinGiver + CTSGameRules' AddObjectiveInList/
  SendObjectiveToHud/TeamWin) is INTERDEPENDENT — CTSObjectivePointer::Use etc. are blocked on the missing CTSGameRules
  objective methods, so that cluster wants doing together (CTSGameRules first).

**File-name parity note** (user asked): `audit.py`'s `norm_file` strips the `ts_`/`TS_` prefix + drops `"manager"`, so
e.g. `ts_animation`≡`TS_animation`, `ts_stunt`≡`TS_stuntmanager` already report ✓. The 22 remaining FILE mismatches are
mostly the original GROUPING many classes into one `TS_*.cpp` (TS_entities/TS_objectives/TS_worldgun) vs our per-class
files — single-class renames are taken inline (corpse/grenades); the grouped merges are deferred per HANDOFF.

**Dossiers saved** (scratchpad, reusable): `dossier-hldmcompat.md` (the 18 missing weapon/ammo spawner classes — entity
name map + Random<X> sets; UNLINKED: CTSPistolWeapon/Ammo; do the CTSGroundWeapon/Ammo base FIRST). `dossier-CTSAnimation.md`,
`dossier-CTSStunt.md`, `dossier-CTSStunt-callers.md`.

### cont.73 cont. — worldgun/hldmcompat cluster (bottom-up): missing classes 24 -> 8

Per the refined goal (do dependencies first, ALL missing-class closures). Commits `875da11`..`86bfb6a`.

- **CTSGroundWeapon 5/14 -> 14/14** (`875da11`, header ts_groundweapon.h): full settle-then-pickup state machine
  (Spawn point-bbox -> Fall -> OnGround SOLID_TRIGGER grab box -> WaitTouch[0x1d auto / else USE] -> GetWeapon),
  Save/Restore + DEFINE_FIELD table (respawntime/tsweaponid/extraclip), ObjectCaps=FCAP_CONTINUOUS_USE,
  CanUsedTroughWalls()=0, virtual GetWeapon for subclasses. **audit.py our_members() fixed** to detect non-m_
  members (the original's FGD/save names are unprefixed) so faithfully-named members aren't falsely "missing".
- **CTSGroundWeapon buy gate** (`cb8330c`): the user flagged unused attackerteam/buyable = missing logic. RE:
  CashBased() is hard-OFF in the shipped game (cash economy compiled out) so the buy gate is DEAD in the original
  too — but PRESENT in GetWeapon. Restored it gated on CashBased() (team restriction + cash price via GiveCash);
  attackerteam/buyable/extraclip now consumed, 1:1. **LESSON: unused members => flag + fix immediately.**
- **10 weapon spawners** (`9d860be`, hldmcompat.cpp): 6 random (CTSPistol/BigPistol/Shotgun/Rifle/SMG/KnifeWeapon,
  Spawn+GetWeapon+RandomX) + 4 fixed (CTSSniper/MG/Katana/GrenadeWeapon, Spawn). Derive CTSGroundWeapon, remap
  stock HL weapon entity names. **Conflict resolved:** the stock SDK LINK_ENTITY_TO_CLASS lines (weapon_shotgun/
  gauss/egon/rpg/crossbow/hornetgun/snark/satchel/handgrenade/9mmAR/ammo_ARgrenades) commented out across 10
  stock weapon files (the original has no stock HL weapons); stock CLASSES stay compiled (hgrunt->CShotgun,
  client->CRpg). No effect on TS maps; stock maps now spawn TS weapons.
- **CTSGroundAmmo 9/11 -> 11/11 + 6 ammo spawners** (`07067a6`, header ts_groundammo.h): m_ammo[16] caliber-indexed
  array (calibers reach 13); CTSPistol/BigPistol/Rifle/SMG/Shotgun/SniperAmmo derive it (RE: CTSBigPistolAmmo::Spawn
  = CTSGroundAmmo::Spawn + m_ammo[5]/[11]). Stock ammo LINKs commented too. ammo amount = TSRoundsPerMag proxy (VERIFY
  the orig get_ammo_info(cal)->default/2).
- **Unlinked pistol keepalive** (`86bfb6a`): CTSPistolWeapon/CTSPistolAmmo are compiled-but-unlinked in the original;
  our --gc-sections DCE'd them -> an exported `hldmcompat_keep_unlinked()` anchor keeps them. 3/3 + 1/1.

### cont.73 cont.2 — more missing-class closures (bottom-up): missing classes 8 -> 5

- **CTSPhysicObject 5/5 + CTSGrenade 2/2** (`a5294db`, ts_physicobject.h): slow-motion physics base (velocity/
  avelocity/gravity scaled by fuser1 when slowed; save/restore; WaitThink toggles on fuser1<1 as the InSlow proxy)
  + the thin grenade subclass (grenbounce sounds + damp/settle). Kept via ts_grenades_keep_physic() until the
  grenade/worldgun hierarchy is rewired to derive them (CTSWorldGun:CTSPhysicObject + thrown grenades) — follow-up.
- **CBulletManager 6/6** (`3f77c1a`): our slow-bullet FREE functions (TSFireSlowBullet/Update/Reset) wrapped into the
  CBulletManager class (FireBullet/Reset/Think/UpdateBullet/UpdateBulletTimeSlice/DestroyBullet); 256-slot ring =
  m_bullets member; global g_BulletManager (orig embeds it in CTSGameRules+0x1864). Callers (ts_gun/ts_gamerules) updated.

**KEEPALIVE pattern** for compiled-but-unreferenced classes (the original keeps them; our --gc-sections drops them):
an exported, never-called function referencing static instances (`hldmcompat_keep_unlinked`, `ts_grenades_keep_physic`).
Honest stopgap until the real hierarchy/wiring is built; flagged in each.

### cont.73 cont.3 — ALL missing classes closed: 24 -> 0 ✅

- **Emitter cluster** (`6317297`): CTSSharedEmitter 10/10 (ts_sharedemitter.h, global s_Emitter + 2x512 arrays +
  the keyvalue parsers; most of the pipeline DEAD as shipped — reproduced inert) + CTSEmitter 4/4 (self-removes in
  Spawn) + CTSParticle 3/3 (registers + persists) + CTSWind 3/3 (BlowThink broadcasts gmsgTSParticleSystem subtype 3).
  gmsgTSParticleSystem (TSPSys,-1) registered; KillAll wired into ServerDeactivate. Client TSPSys decode = separate track.
- **CInfoBreakable** (`6eff724`): the breakable respawn retry-helper (1/1, ts_func_break.cpp). FOLLOW-UP (not wired —
  playtest-sensitive): CBreakable::BreakableRespawn (@0x74e84 box-clear retry) + Die creating/arming the helper +
  the respawn-time members. Kept via ts_func_break_keep() until then.

**🎯 ALL 24 originally-missing TS classes are CLOSED (24 -> 0).** audit.py headline: 0 missing / 0 invented classes.

**Remaining audit work** (drifted, not missing): ~131 "invented methods" to review (mostly legit SDK helpers on
CBasePlayer/CBaseEntity); the big drifted classes' MISSING methods (CTSGameRules 12/39, CTSGun 28/47, CM61Grenade,
CTSFlag, CTSWorldGun 4/13 -> rederive CTSPhysicObject, CBreakable 17/22 -> the breakable-respawn cluster above);
23 file mismatches (grouping divergence, mostly OK). KEEPALIVE'd classes (pistols, CTSPhysicObject/CTSGrenade,
CInfoBreakable) want their real hierarchy/wiring built to drop the anchors. **⚠ EVERYTHING this session is
UNPLAYTESTED** — the whole worldgun/hldmcompat/bullet/physics/emitter behaviour needs a playtest pass.

**(old NEXT, now done):** the emitter cluster — **CTSSharedEmitter (10-method particle manager) FIRST**, then
CTSEmitter/CTSParticle/CTSWind entities (TS_emitter.cpp; dossier-emitter.md). CInfoBreakable (1 method, created by
CBreakable::BreakableRespawn — do with the breakable-respawn drift). Then the big drifted classes (CTSGameRules 12/39,
CTSGun 28/47, CM61Grenade, CTSFlag, CTSWorldGun 4/13 -> rederive CTSPhysicObject), 131 invented methods to review, and
client-side structural parity (separate manual track). ⚠ The whole worldgun/hldmcompat/bullet/physics behaviour is
UNPLAYTESTED — needs a pass (stock-name weapon remap, pickup physics, slow-bullet, grenade bounce).

## 2026-06-28 (cont. 72) — RE-tooling pass: audit toolchain + deterministic inventory (committed); pump WIP

Built the machine-checkable RE inventory + bidirectional audit, made it deterministic, set up the Ghidra MCP.
10 commits `77cb586`→`1f839c8` on main. **NEXT: use audit.py to restructure our code to the original layout
(source parity).**

- **audit.py** — bidirectional ours-vs-original structure check against ts_inventory.json (file/class/method/
  member/global, MISSING + INVENTED). `audit.py <Class>` per-class; `--strict` CI gate; wired into run.py →
  AUDIT.md. Current: 24 missing classes, 0 invented classes, 145 invented methods (review), 211 missing methods,
  54 missing named members, 25 file mismatches, 24 missing globals. coverage.py still does the external contract.
- **vrefs.py** (`re.sh vrefs`) reverse virtual-call finder; **curate_members.py** producer for members_re.tsv
  (the orphan); **client_members.py** (binmap-match our client.so→client.dll + ApplyClientThis.java + DumpMembers)
  → members_auto_client.tsv (51 classes/1319 members incl. CHud*); **binmap.py** .so↔mp.dll/client matcher.
- **member-inventory completeness**: DumpMembers parses the opaque `*(t*)(this+0xNN)` form → recovers
  CTSGameRules/CTSStunt/CTSAnimation (860→1030/137). **s_weapon_info/s_weapon_status structs** applied (weapon
  code reads this->m_pActiveInfo->iFireType). **ts_inventory.json** now carries full per-class members (1402) +
  client skeleton.
- **DETERMINISM** (was wobbling run-to-run): cached single-threaded `re-base` (`re.sh analyze`, COPIED per setup,
  also faster) + `-max-cpu 1` (member SET) + CANONICAL type column in DumpMembers (Ghidra's decompiler integer-type
  inference is non-deterministic, unreachable by -max-cpu; float/double/ptr stable → presence-keyed, integers→int).
  Verified 0-diff. Diagnosed by measurement after 3 wrong hypotheses.
- **Ghidra MCP** front-end on the typed `re` project (CLAUDE.md §4a.3 + .lock gotcha; `[[ghidra-mcp-vs-own-tooling]]`;
  `scripts/ghidra/mcp-up.sh`).
- **Pump shotgun (COMMITTED `2b23000`+`9c3ec0c`, pushed)**: RE-confirmed HELD/auto-alternate (all 3 default to
  PUMP; ShotgunFire alternates fire/rack via item+0x24 = predicted m_fInSpecialReload; tap=one step). Rack sound
  CHAN_ITEM. Pump VKickback 0.35× (benelli/spas pump mode; mossberg full) — applied to the CLIENT aim-recoil
  (TS_AddViewRecoil), NOT server VKickback (RE @0x70414: punchangle is bots-only); the magnitude rides to the
  client via the fire-event fparam2. Playtest-confirmed.
- **Client-side architecture divergence noted (restructure)**: original splits the CLIENT fire path into
  per-fire-type handlers (FUN_10040c20 pump / FUN_1003f090 akimbo / FUN_1003dfa0 standard); we use ONE EV_TSFire.
  audit.py is SERVER-ONLY → won't catch client structural divergence; client parity is a separate manual track
  (dc.sh client + clientdll project). See HANDOFF cont.72.

## 2026-06-27 (cont. 71 cont.2) — long live-playtest pass: laser/HUD/muzzle/weapons + pump (in progress)

Marathon interactive session, user firing every change in graphical xash. Committed `8c27afa`→`0b45726` on main
(see HANDOFF "⭐⭐⭐⭐⭐ START HERE cont.71 cont." for the per-commit list). Highlights:
- **Lasersight**: local player's dot is a HUD dot (RE `FUN_1003d600` skips local; others get the world beam+dot);
  beam = `laserbeamanim.spr`; dot screenspace-anchored on the eye-ray (distance-independent, centred up close).
  Map-change persistence fixed (don't clear `g_playerAttach` on VidInit). Per-weapon attachment buy-binding fixed
  (interleaved tkw/tki). `useitems` = ATTACHMENT toggle (RE `FUN_0x71564`), akimbo swap = right-click.
- **Viewmodel aims UP with ground move speed** (cl_dll/view.cpp `V_CalcRefdef`) — for every weapon; the laser dot
  follows the model. Halved the figure-8 weapon sway. Flashlight = light-only + `flash.spr` on the gun, radius
  1.5×. Muzzle flash = `gun_muzzle.spr`(sil)/`gun_muzzle2.spr` for all weapons.
- **HUD**: numbers.spr digit font (health/powerup/bottom-box ammo/top-bar slots); bottom-box attachment list with
  item_installed/not_installed dots + lighter-background active highlight; melee BLOCK bar (gmsgKFuPower stamina,
  melee-only, fades to red).
- **Weapons**: katana right-click throw + stuck-katana NOT pickup-able + throw caps (knife 5/seal 10/katana 1);
  all melee block on RELOAD; weapon UNHOLSTER anim on switch (per-weapon draw-seq parsed from viewmodels +
  viewmodel-change replay force for same draw-index pairs); shotguns trace ALL pellets client-side (8 decals).
  Spectator top-left inset (pip) forced OFF.
- **RE-confirmed faithful (no change)**: UMP 2-round burst (`burst=(type!=0x17)+2`); double-fire = 1.5× (`*=1.5`).

🟡 **IN PROGRESS (uncommitted): pump-action shotgun.** Fire is now press-edge (one shot per pull) — WORKS. The
cosmetic RACK (pump anim+sound between shots) was reworked from a custom timer (failed: custom fields aren't
decremented in client prediction) to a `bool m_bNeedRack` + the predicted `m_flTimeWeaponIdle`; NOT yet
playtest-confirmed. ⚠️ Lesson (cost two regressions): NEVER gate the FIRE on non-predicted state (an `m_bPumped`
chamber state machine desynced → infinite-fire); keep pump cosmetics cosmetic. ALSO: ts_gun.cpp builds into BOTH
hl.so + client.so — always `build.sh amd64 all`, else the client predicts stale weapon logic. [[useitems-is-attachment-toggle]]

## 2026-06-26 (cont. 71 cont.) — HUD numbers + attachment toggle + katana + melee block bar (live-playtest pass)

Long interactive playtest pass, all RE-grounded + user-confirmed. Committed as a client/server pair
(cl_dll HUD presentation; dlls gameplay) on top of 8c27afa.

- **numbers.spr digit font** wired into: health number (ts_status), powerup seconds (ts_powerup_hud),
  bottom weapon-box ammo "clip / reserve" (ts_ammo, TSHud_DrawNumberPair with a slash), and the top
  weapon-bar slot numbers 1-5 (ts_weaponbar). Left as text: the top-bar cursor x/y (per user). Helpers
  TSHud_DrawNumber/NumberWidth/DrawNumberPair in ts_hud (font map sprites/hud.txt number_0..9, 11px).
- **`useitems` = ATTACHMENT TOGGLE** (NOT akimbo swap — user corrected; akimbo swap is right-click).
  RE'd FUN_0x71564: item +0x10 = EQUIPPED mask, +0x14 = ACTIVE mask. State machine (both laser+flash
  equipped): active cycles 6(both)->0(off)->2(laser)->4(flash)->6; one equipped toggles it. Rewrote
  CTSGun::SwitchItems to this; moved the akimbo barrel-swap to SecondaryAttack (right-click) as
  SwitchBarrel. AddItem starts new toggleables ACTIVE. Wire: ActItems now carries the ACTIVE mask (gates
  the laser beam render + dive-reticle distance); WeaponInfo flags byte carries the EQUIPPED mask. Bottom
  weapon-box restructured top->bottom (name/caliber/firemode/ammo/attachments/bar) listing each equipped
  attachment with a laserdot.spr dot, active ones bright. [[useitems-is-attachment-toggle]]
- **Katana right-click throw** (orig SecondaryAttack fire-type 10 -> KatanaThrow): generalized ThrowKnife
  for katana (katana/miss.wav) + routed it. **Stuck katanas NOT pickup-removable** (the pickup was our
  added nicety; original leaves a settled blade inert) — gated BecomePickup so a katana stays inert
  decoration; knives/seal keep the pickup. **Throw caps**: knife 5, seal-knife 10, katana 1 (ts_weapon_info).
- **Melee block bar** = MELEE BLOCK STAMINA (field_0x774), shown ONLY for melee weapons (kungfu/knife/
  katana/seal), NOT health, NOT on guns. RE CCombatBlock@0x85ebc + UpdateMeleeBar@0x9650c: ALL melee
  block on RELOAD (was kungfu-only); each block drains via UpdateMeleeBar; networked via **gmsgKFuPower**
  (CHAR 0..100); the client (ts_ammo) draws it as the bottom-box bar gated on melee. The bar colour FADES
  from the HUD colour (full) toward red (empty), not a threshold switch (TSHud_HealthBar). Top weapon-bar
  shows melee as count "x" only (not "x/max"). Tuning (user-playtested): re-block cadence +0.5s (RE fc_0_5,
  was 0.8), drain 33 (RE), continuous regen ~25/sec (never pauses, even while blocking).

Files: cl_dll/{ts_hud,ts_status,ts_powerup_hud,ts_weaponbar,ts_ammo,ts_attachments}.{cpp,h};
dlls/{ts_gun.{cpp,h},ts_knife.cpp,ts_weapon_info.cpp}. Builds clean (glibc 2.14). [[match-original-names-for-coverage]]

## 2026-06-26 (cont. 71) — lasersight map-change loss FIXED + per-weapon attachment binding FIXED (live-confirmed)

Two laser/attachment bugs the user reported, both RE-grounded + live-playtest-confirmed (UNCOMMITTED, on top of 514a50a).

1. **Lasersight/flashlight lost on MAP CHANGE** — the `ActItems` broadcast is change-detected on
   `CTSGun::m_iClientAttach`, whose cache SURVIVES a changelevel (the gun persists) so it never re-sends;
   the client was CLEARING `g_playerAttach` on `TS_Attachments_VidInit` → desync → laser/flashlight +
   dive-reticle distance gone after a map change. **Fix = the inverse of "force a re-send":** client.so
   is not reloaded across a map change, so the static `g_playerAttach` stays in sync with the surviving
   server cache as long as we DON'T clear it. Removed the `memset(g_playerAttach,…)` from
   `TS_Attachments_VidInit()` (kept the SPR-handle reset — those genuinely go stale). The original keeps
   its array likewise (a static global never cleared on level change), which is why it lacks this bug.
   (The prior `m_iClientAttach=-1`-in-Deploy attempt was the wrong fix — re-fired every deploy, broke
   fresh-buy; no server change is needed.) [[map-change-state-reset]]

2. **Attachment landed on the WRONG weapon** ("SOCOM+laser → the SHOTGUN gets the laser, and no SOCOM").
   Root cause: the buy menu `Apply()` (cl_dll/ts_buymenu.cpp) sent ALL weapons in one `tkw` then ALL
   attachments in a separate `tki` afterward. The server appends to `m_armament[]` in command order and
   `BuyArmament` applies via `CBasePlayer::AddItem` → the ACTIVE `CTSGun` (active = LAST weapon bought),
   so every attachment bound to the last weapon, and the per-weapon `Slot::attachMask` binding was lost.
   RE confirmed the original `BuyArmament@0x83f10`/`AddItemToArmament@0x83ea4`/`AddItem@0x8440c` apply the
   exact same active-weapon way — fidelity hinges on QUEUE ORDER. The original CLIENT (`FUN_10039400`)
   runs a sort-then-emit pass that sequences each accessory right after its weapon. **Fix:** `Apply()`
   now emits INTERLEAVED — each weapon's `tkw <id>_<n>` immediately followed by its own `tki <bit>` items
   (one tki per bit so `CTSGun::AddItem` validates each against that weapon's supported/built-in mask).
   Per-command emission also fixed the SOCOM going missing: the old single 256-char `tkw` string
   truncated/overflowed on a full loadout and dropped weapons. reference/BUY.md model unchanged (names/
   formats faithful); only the emission ORDER corrected. [[match-original-names-for-coverage]]

Verified by adding temporary `[TSATTACH]` server ALERTs (Deploy type/attach + broadcast idx/type/attach),
reading the trace from the live xash run, confirming the queue order + broadcast, then REMOVING the debug.
Builds clean (hl.so + client.so, glibc 2.14). HANDOFF open-work #1 (laser map-change) is DONE.

## 2026-06-26 (cont. 70 cont.2) — DIVE RETICLE reconstructed + numbers.spr font (UNCOMMITTED; laser-distance has a known map-change bug)

New client HUD **cl_dll/ts_reticle.{cpp,h}** (`CHudTSReticle`, wired in hud.h/hud.cpp + cl_dll/CMakeLists.txt).
The TS dive reticle: a center DONUT crosshair + a left "(" arc of 3 dive-step thirds colored by loadout
weight, with a dot tracking aim pitch. RE foothold (live trace on the original, see below):
- Sprites loaded by client.dll FUN_1004cb50 into a STATIC object @ client.dll+0xd0ca0 (handles read live:
  cross_e=0x3b/cross2=0x3c/cross_t=0x3d/divestept=0x3e/divestepc=0x3f/divestepb=0x40). Dims: divestep 40x80,
  cross 64x64. The DRAW funcs (FUN_1003254e @0x3254e, FUN_10033a6b @0x33a6b) decompile to GARBAGE (Ghidra
  lost the frames — reached via register/tail-call dispatch), so colors/positions were TUNED against the
  user's screenshots, not byte-RE'd. Full map: reference/STUNT.md? no — scratchpad/RETICLE_RE.md.
- Crosshair = **cross2** (donut, transparent bg) via SPR_DrawHoles tinted near-black (cross_e is the same
  art on a BLACK bg → only additive; cross_t is an X). NSEW ticks + are drawn as FillRGBA lines (2x4 / 4x2,
  the dot's base blue), pulled IN when a lasersight is equipped. NO center dot.
- 3 dive thirds (divestept/c/b) drawn OVERLAID at one position (each canvas holds its third) — blue if that
  dive tier is available, red if too heavy (CanDive High/Med/Low = free slots >39/29/19, from maxspeed via
  GetClientMaxspeed + the cont.69z slot formula). The aimed-at AVAILABLE third brightens; red never does.
- Pitch DOT rides a CIRCLE (R=33) around center: a 60-deg view cone (+-30 pitch, arc angle = pitch*3) maps
  to the arc's 180deg (30 up->top, level->left, 30 down->bottom). Dot is always the bright blue (104,169,240).
- Reticle hidden in spectate (g_iUser1 / IsSpectateOnly); ALSO suppressed the stock valve crosshair in
  spectate (hud_spectator.cpp SetCrosshair(m_hCrosshair) -> 0 in both OBS_IN_EYE/ROAMING branches).
- **numbers.spr sprite-font**: TSHud_DrawNumber (cl_dll/ts_hud.{cpp,h}) — digit imagemap rects from
  sprites/hud.txt (number_0..9, 11px each). Used for the lasersight DISTANCE readout (PM_TraceLine from the
  eye along v_angles, dist/52 = metres, drawn right of the reticle, gated on TS_LocalHasLaser).

🔴 OPEN / BROKEN:
- **Lasersight attach is lost on MAP CHANGE** (g_playerAttach reset client-side; server ActItems is
  change-detected on CTSGun::m_iClientAttach which survives the level → no re-broadcast). A `m_iClientAttach
  = -1` in Deploy() REGRESSED it (laser gone on fresh buy) → reverted. Needs a re-broadcast that doesn't
  break first-deploy. See [[map-change-state-reset]].
- **numbers.spr NOT yet wired into health / ammo-max / powerup (2/3/5)** — user asked; still text/other.
- Lasersight reticle should gate on the laser's ACTIVE toggle (useitem; starts on) not just equipped —
  deferred. Distance divisor /52 + the number position are tuned guesses.

[[live-original-ts-debuggable]] [[match-original-names-for-coverage]]

## 2026-06-26 (cont. 70 cont.) — CTSStunt FULLY RE'd + reconstructed + live-playtest iterated (COMMITTED)

Committed the prior cont.70 checkpoint as **b8080fb** (gun attachments/powerups/slow-mo/damage sounds),
then did the user's goal: "faithfully reconstruct via RE the ENTIRE CTSStunt class and how it works".

LIVE-PLAYTEST ITERATIONS (each grounded in RE after the user flagged guessing) — committed together:
- **gravity/friction were SWAPPED + view_ofs mislabeled as punchangle.** Verified via progdefs.h + the
  offset map (gravity@0x11c, friction@0x120, view_ofs@0x174, punchangle@0x68 — confirmed against
  velocity@0x20/v_angle@0x74/button@0x180/flags@0x1a4). Swapped gravity<->friction + punchangle->view_ofs
  across all stunt methods. This fixed the "random gravity" (GoDive sets gravity=0.75 floaty arc on every
  dive; friction=0.25 on a low dive) AND revealed the PRONE VIEW source: Go* set pev->view_ofs=(0,0,0)
  (the g_vecZero->pev+0x174 I'd misread). The -8 in PM_CheckParamters is the DEATH cam (gated on dead).
- **Play dispatch must run AIRBORNE** (orig gates only ladder/water, not FL_ONGROUND) — my onGround gate
  killed the dive mid-air. **m_flLastStunt stamped every frame on ground** (PreThink @0x808cc): time since
  leaving ground < 0.7/timescale = the mid-air dive window (jump->dive + run->dive).
- **TSHandle damps the CMD, not velocity** (the __DT_REL slots = cmd.forwardmove/sidemove/upmove): SLIDE ->
  forwardmove=0 + side/up*0.3 (strafe-only + slow; your landing velocity carries you = the slide), STOP ->
  *0.5. I'd been zeroing velocity (killed the slide). **Landing slide-vs-roll** gated on JUMP/ALT1/DUCK
  (0x4006), not WASD -> you slide every dive landing.
- **PM_Jump returns while prone** (iuser4 & PRONE|SLIDE|DIVE|WHEEL|WALLJUMP_A|WALLCLIMB_A) -> jump never
  hops while prone, only triggers the getup. **CheckForWallClimb** gated on a JUMP-press edge + 0.3s window
  (NOT FL_ONGROUND — my bug broke wall-scaling); WALLCLIMB_A set -> hold-jump auto-climbs.
- **GETUP**: STANDUP bit 0x8 is NEVER set in the binary (TSStandUp dead). Reconstructed as a shared timed
  two-stage (m_flRollTime=0.75 from HandleSlide-end): intermediate eye 16 + getup anim (dive_<dir>_end) +
  STOP(slow)+free move, then at timer-end clear PRONE -> end eye = (DUCK held ? 12 duck : 28 stand), PM_Duck
  owns the hull from there. (Exact settled-prone getup trigger not 100% pinned; routed through this path.)
- **Animations**: wired CTSAnimation::Animate stunt codes to the model's real sequences (dive_<dir>_<grip>,
  dive_<dir>_end, slide_<dir> = the prone poses, crawl, wheel_*/lowwheel_*, walljump_climb/end, morfy_jump,
  bullettime). The slide_* set IS the lying-prone set (no separate prone idle).
- **Full RE map written: reference/STUNT.md.** New files: dlls/ts_stunt.{h,cpp}, reference/STUNT.md.

RE (complete map in **reference/STUNT.md**): decompiled all 25 CTSStunt methods (Init/Play/HandleStunt/
GoDive/GoRoll/GoWallClimb/GoWallJump/GoBulletTime/GoWheel/HandleDive/HandleSlide/HandleWheel/HandleWallJump/
HandleWallClimb/Landing/CanDive{High,Med,Low}/NextDive/SlotBonus/CheckForWall{Jump,Dive,Climb}/CheckForBreakables)
+ the pm_shared stunt family (TSHandle@0xe96a0 dispatcher, TSGoDuck, TSStandUp, TSDive, TSWallJump,
TSWallJumpClimb, PseudoClimb, CanJumpHigh) + PM_PlayerMove/PM_Duck. **THE KEY FINDING that unblocks it:**
the system is driven by ONE networked field **pev->iuser4** (entvars 0x250 -> pmove->iuser4 @0x208). The
engine round-trips iuser4 pev<->pmove (server) and networks it (clientdata->TxferLocalOverrides->entity_state
->prediction pmove, verified cl_dll/entity.cpp:87/210), so **pm_shared TSHandle() (compiled into BOTH hl.so +
client.so) PREDICTS the dive/prone small hull (usehull=1) + low view (VEC_DUCK_VIEW) + dive/wall launch
velocity on both sides** — exactly why the old DLL-only FL_DUCKING/UTIL_SetSize/view_ofs forcing never stuck
(the predictor recomputed the hull every frame). iuser4 bit map in reference/STUNT.md §1.

IMPL: new **dlls/ts_stunt.{h,cpp}** = real CTSStunt class (member CBasePlayer::m_TSStunt, Init in Spawn,
Play() from PreThink) writing pev->iuser4. New pm_shared hooks in **pm_shared/pm_shared.cpp** (TSHandle +
TSGoDuck/TSStandUp/TSDive/TSWallJump/TSWallJumpClimb; TSHandle() called from PM_PlayerMove before PM_Duck;
PM_Duck gated off while any stunt flag set). Removed the ad-hoc m_bDiveRolling/m_flWallClimbFuel/m_bProneSize
PreThink approximations from player.cpp (members left declared, now unused). Added TSPlayerFreeSlots() to
ts_gun.cpp (ts_weapons.h) for CanDive weight gating (lighter loadout = higher dives). ts_stunt.cpp in
dlls/CMakeLists.txt. **Builds clean (hl.so + client.so, glibc 2.14)**; CTSStunt + TSHandle linked both sides;
synced to the xash mod root. NOT committed — awaiting the user's live playtest (per workflow).

SLICES/notes: 3rd-person stunt POSES (CTSAnimation::Animate 0x28..0x39) still deferred — the TS specialist
player models aren't in this data extraction (Animate calls are made but no-op). gmsgTSNextStunt (stunt-meter
HUD byte) NOT sent (no client handler yet). PseudoClimb (ladder-style wall-grab refinement) stubbed/omitted.
Wall-jump up-speed (270) tuned (orig reads a .rodata const that read 0 at the computed offset). CheckForBreakables
(dive melee-shove) left to the existing TSCheckRunThrough. [[live-original-ts-debuggable]] [[match-original-names-for-coverage]]

## 2026-06-26 (cont. 70) — attachments + powerups + slow-mo/damage sounds + stunts (**UNCOMMITTED ON DISK**)

Very long interactive playtest session, NOT committed (21 modified + new `cl_dll/ts_attachments.{cpp,h}` on
top of `6ecf77b`; builds clean glibc 2.14). Full detail in HANDOFF.md "⭐⭐⭐ START HERE (cont.70)". Summary:
- **Gun attachments #11 + scope/laser/flashlight**: silencer (fire-sil + muzzle suppress; `fire.sc` iparam2=
  attach mask), scope (FOV cycle on secondary; AUG/Barrett built-in via weapon_info +0xe0/+0xe4 = new struct
  fields), lasersight+flashlight (NEW `ts_attachments.cpp` — original `ActItems` msg, TriAPI beam+dot/dlight).
- **Buy menu**: bit-index/value fix (display + mouse), green Buy/Akimbo, akimbo combine + both-icons + step-down.
- **Powerups**: super-jump (m_bLowGravity) + kung-fu (m_iKungFu) independent + bottom-center white HUD; kung-fu
  reload BLOCK (CCombatBlock window + VictimKungfuHold negate+stun). Double-fire → all weapons full-auto.
- **Slow-mo**: goslow.wav (EV_PlaySound, RE FUN_1004bef0) + 4 fading heartbeats; tint bluer.
- **Damage sounds**: bodyhit/headshot/kevlarhit (TraceAttack), pain1-4 (Pain), whizz1-5 flyby (closest-pt-on-seg).
- **Bugs fixed**: impact-ric "hear on approach" (was EMIT_AMBIENT_SOUND on world = static sound; → client
  entity-0 positional); map-change sprite-cache staleness (TSParticles_Reset + TS_Attachments_VidInit on VidInit).
- **🔴 INCOMPLETE — CTSStunt (roll/wall-climb/+alt1 dive/prone)**: ad-hoc PreThink approximations, NOT faithful.
  BLOCKER: dive/prone needs a client-PREDICTED small bbox+low view; DLL forcing (FL_DUCKING/UTIL_SetSize/
  view_ofs) is overwritten by pm_shared each frame → **needs pm_shared reconstruction gated by pev->iuser4
  stunt flags** (RE'd: HandleStunt@0x8b000, GoDive@0x8b1a0 sets iuser4 0x4/0x10+height bits, GoRoll@0x8cfe8,
  GoWallClimb@0x8aa1c). Decompiles in scratchpad/. The 3rd-person poses need the TS player models (not extracted).
- **🔴 PENDING (not started)**: glass double-pane penetration (RE the caliber gating + damage/decal-behind),
  golden-colts no-fire-sound-until-reload (prediction sync), wall-climb weight formula.
[[live-original-ts-debuggable]] [[flag-unfound-re-early]] [[match-original-names-for-coverage]]

## 2026-06-26 (cont. 69z cont.) — weapon-feel + powerup + grenade + impact-effect pass (COMMITTED, live-playtested)

Long interactive playtest session; all RE-grounded. Committed to `main`. Items:

WEAPON SLOTS / MOVE SPEED (RE GetSpeedBySlots@0x66bd4, GetAccelBySlots@0x66c4c, slot cost = s_weapon_info+0xd0
byte): recovered all 38 per-weapon slot costs -> s_weapon_info.slotCost (knife 1, pistols 10, SMG/uzi 20,
rifle/shotgun 40, barrett/m60 70...). CBasePlayer base maxspeed = 210 + (120/81)*remaining (remaining = 81 -
sum of owned slotCost) -> TSSlotBaseSpeed (ts_gun.cpp), wired into player.cpp PreThink as the base that slow-mo
fuser1 scales. PICKUP GATE: AddWeapon -> bool, refuses a fresh weapon that won't fit 81 slots (GiveTSWeapon ->
NULL -> ts_groundweapon leaves it on the floor); already-owned weapons aren't re-picked-up except akimbo combines.
(Buy menu encumbrance was an explicit SLICE; slot cost is +0xd0, price is +0xd8 — distinct.)

DOUBLE-FIRERATE (RE PostThink@0x81610): the powerup is 1.5x NOT 2x (player+0x724&0x10 -> weapon-timer decrement
*= 1.5). Was an over-fast invented 2x (flRate*=0.5). Now flRate/=1.5 (predicted). Still makes semis fire-while-held.

HEALTH POWERUP DECAY (RE PreThink@0x808cc, player+0x73c = m_flHealthRegen): over-heal bleeds 1 HP/sec back to
max_health then disarms. Was set but never read; wired in player.cpp PreThink.

SLOW-MO SOUND PITCH: split TSSlowPitch (normal-recorded combat sounds, 100*perceivedSlow, capped 100) vs
TSRecordedSlowPitch (recorded-slow sounds — gun-fire + bullet-hit ric — 100+300*perceivedSlow, 1..255, sped up
at normal play, deepening in slow-mo). Applied to: thrown-knife impacts (ts_knife.cpp), per-shell shotgun racking
(weapons/insert-shell.wav, restored — the ShotgunReload rewrite had dropped it), and the bullet-impact ric.

SLOW BULLETS (ts_bulletmgr.cpp): per-caliber RE'd base speed (40*ammo_info[+0x24], kBulletSpeedUnit table) instead
of the flat 6000; faithful fuser1 slow factor (reverted a user-flagged "too slow" ramp deviation). Visual: a
short bullet_trail.spr dash that RIDES the bullet head (RE FUN_10035010, speed-scaled 5..25u) + shotgun_pellets.spr
head sprite (RE: original draws it via FUN_10050080) at native amber. Bullet-hit ric = debris/ricN, ATTN_STATIC.

GRENADE (RE CM61Grenade Spawn@0x7c974 / WaitThink@0x7cacc / BounceTouch@0x7ca90 + TSExplosion@0x6d01c): rewrote
CM61Grenade — swept WaitThink (trace 64u ahead, shatter anything with health <= 0.1*speed -> flies THROUGH glass
like the knife, not break-then-bounce), fuse counts in SLOWED time, detonates via TSExplosion. TSExplosion = a
FUNCTION (screen-shake + radius damage w/ linear falloff + ScreenFade flash on players); visual = TE_EXPLOSION
(SLICE: original's gmsgTSCreateDebris material-1 client fireball + slow-mo-point-on-spectacular-kill deferred).
Grenade ammo = have/don't-have (clip 1): throwing the last one drops the slot + GetBetterWeapon (like the knife).

BULLET IMPACT SHOWER + DECAL COMPAT (the big RE/compat fix): the ~50-dot "shower" was the ENGINE's
TE_GUNSHOTDECAL (our server DecalGunshot -> UTIL_GunshotDecalTrace sends 0x6d) = decal + R_BulletImpactParticles
burst + engine ric. VERIFIED in the binary: original ShootBulletsPlayer@0x5f69c lays NO server decal/TE at all;
the decal is CLIENT-side (client.dll DamageDecal FUN_10001000 picks {break/{shot/{bproof1/{splat by
pe->classnumber(+0x8c)==1 / pe->rendermode(+0x68)==0, then R_DecalShoot). So our server-side decal broke drop-in
compat (double vs original client, none vs original server). FIX: removed the server bullet decal (entry+exit) and
lay it CLIENT-side in EV_TSBulletImpact mimicking FUN_10001000 (breakable->{break, normal->{shot, else->{bproof1).
Decal-name gotcha (TS decals.wad): bare "{shot"/"{splat" don't exist (numbered only), bare "{break" is a DEBUG
"BREAK"-text lump — so use {shot1..5 and {break1..3, "{bproof1" bare. [[match-original-names-for-coverage]]
[[live-original-ts-debuggable]]

## 2026-06-25 (cont. 69z) — #14 right-click HOLD-POSITION toggle + #13 shotgun RELOAD state machine (live-playtested)

Both RE-grounded via typed Ghidra decompiles of the originals (re.sh) + viewmodel sequence dumps +
binary FP-constant reads. Built clean (hl.so + client.so, glibc 2.14) and synced to the xash mod root.
NOT yet committed — awaiting the user's live-playtest confirmation.

#14 HOLD-POSITION (RE CTSGun::SecondaryAttack@0x717a8): decompiled the whole secondary dispatch —
grenade/knife/katana/flag throw (fire-type 2/3/0xa/0xb) first, then akimbo modes 4/5 -> Fire(other gun),
then else: scoped (item+0x10 attach & 8) -> Zoom/FOV, else HOLD-POSITION TOGGLE gated on wi+0x28c != -1.
The toggle flips item+0x2c, plays the model's change/rechange transition (the +0xe field of the leaving
anim block), re-points the active anim block item+0x28 between wi+0x252 (normal) and wi+0x28c (hold), and
re-arms timers primary +0.3 / secondary +1.0 (verified doubles @0xf7fb8=0.3, @0xf6df8=1.0). KEY FINDING:
this is NOT just mk23/SOCOM — viewmodel dumps show paired stance sets (change/rechange + alt idle/shoot)
on mk23, desert, ak47, m4, m16, mp5k, uzi, mossberg, sawedoff. mk23 hold-idle = idlec(5) matches the
RE'd wi+0x28c=5. IMPL: s_weapon_info.iHoldChange/iHoldRechange/iHoldFireAnim (per-weapon seq indices from
each v_*.mdl), TSItem.holdPosition, CTSGun.m_bHoldPosition (synced to client predictor via weapon_data
iuser4, persisted per-item in Sync/LoadActiveFromItem). Toggle added to ItemPostFrame IN_ATTACK2 block
(non-akimbo + iHoldChange>=0); held-stance fire anim swapped in StandardFire (iHoldFireAnim). SLICE: scope
zoom branch + reload-in-hold-stance anim swap not wired (we don't have scope-attach plumbing yet).

#13 SHOTGUN RELOAD (RE ShotgunReload__6CTSGun@0x84aac + Reload@0x84d5c + StopReload@0x81288): Reload routes
fire-type 1 (PUMP: benelli/spas12/mossberg) straight to ShotgunReload, a 3-phase machine on item+8:
phase0 open (play seq, lock 0.3) -> phase1 insert 1 shell every 0.3s (wi+0x14=0.3, verified) until tube
full / reserve dry -> phase2 close/pump, end, secondary cooldown +3.0 (dc_3). The anim seqs are sent as
event fparam1 = 8/9/10 (phase codes the client remaps per-weapon, NOT raw indices — benelli reload1/2/3 are
at 2/3/4, mossberg at 8/9/10). reload1/2/3 are CONSECUTIVE in every pump v_*.mdl and our iReloadAnim already
= reload1 (open), so insert=+1, close=+2. THE BUG FIXED: old code replayed the OPEN seq every shell and never
played CLOSE -> gun stuck on the open pose ("held up, no shell, stays til fire"). New: CTSGun::ShotgunReload()
3-phase, driven each frame from ItemPostFrame while m_fInReload && pump; fire interrupt resets m_shotgunPhase
(StopReload); m_shotgunPhase reset on deploy (LoadActiveFromItem). Constants: dc_0_3=0.3, dc_3=3.0, wi+0x14=0.3
(disambiguated from a Write_mp5k spill via the +0x18 fire-rate store = mossberg 0.25). [[match-original-names-for-coverage]]

---

## 2026-06-25 (cont. 69y) — POWERUP / SLOW-MOTION / SLOW-BULLET subsystem + weapon fixes (commit 635c9ca)

Big session after the kung-fu kicks (below). All RE-grounded via 3 subagent RE passes (maps in scratchpad/):
POWERUP_RE.md, SLOWMO_PRESENTATION_RE.md, bulletmgr.c, ccombatattack.c. Committed `635c9ca` on `main`.

POWERUPS (RE POWERUP_RE.md): held timed powerups (slowmo 1 / slowpause 8 / dblfire 0x10) share ONE held slot
(player+0x728) + a double-tap-+use activation (client-side, CL_CreateMove 0.30s, cvar cl_usepowerup). Built:
cl_dll/ts_powerup_hud.{cpp,h} (green top-left "Slow Motion N" via PwUp msg = WriteShort(held)+WriteByte(dur)
from UpdateClientData; PwUpTime countdown); double-tap in cl_dll/input.cpp; UsePowerUp/UpdatePowerUpState in
ts_player.cpp; SlowMotionFlow (self 0.6/0.8 + LOS enemies 0.35/0.225 every 0.5s) + DeactivateSlowMotionFlow.
Pickup = TOUCH only (CTSPowerUp::WaitTouch, no FCAP_IMPULSE_USE — the IN_USE there is an optional swap). Mutual
exclusion is structural (single held slot overwritten on pickup).

SLOW-MOTION TIMING (RE SLOWMO_PRESENTATION_RE.md — the key doc): the slow factor lives in pev->fuser1 (CURRENT,
networked entvars float), eased toward m_flSlowMotionGo (TARGET, set by GoSlow) via InterpolateSlowMotion
(min(ft*7,1) step, 0.05 snap). fuser1 scales: maxspeed (movement), pev->framerate=max(fuser1,0.01) (animation),
AND the weapon-timer decrement (player.cpp PostThink + client prediction hl_weapons.cpp, both reading fuser1)
→ fire rate slows. ×1.5 if dblfire. SOUND PITCH: guns 100+300*slow (ev_ts.cpp; the wavs are "recorded-slow" so
~255 at normal → down in slow-mo), melee 100*slow (ts_gun.cpp TSMeleePitch; normal-recorded → slowed not sped).
SCREEN TINT: the original hijacks the engine SCREEN-FADE in V_CalcRefdef — RGBA(0,100,125) teal (NOT a green
fill — first RE pass wrongly said "no tint"), alpha (1-slow)*50, cvar cl_slowfade. TSSlowMo msg = WriteCoord(100*
PERCEIVED-world-slow) = the others-scale 0.35/0.225 (NOT the 0.6 self), drives the client smoothed g_flClientSlow.

SLOW-MO BULLETS (RE bulletmgr.c): dlls/ts_bulletmgr.cpp ≈ CBulletManager (256-slot ring). ShootBulletsPlayer
gate `rbullet = shooter slow < 0.99` → each pellet becomes a flying bullet instead of instant hitscan. Per
frame (CTSGameRules::Think): advance by GetBulletVelocityMult(slow,pauseFlag)*6000*frametime (slow-mo ≈ slow
= fast tracer; slow-pause = slow/((1-slow²)*10) ≈ dodgeable), trace (dont_ignore_glass → HITS glass), damage +
DecalGunshot on hit, else fly on; vanish when slow ends. Visual = a CBeam sprites/bullet_trail.spr extending
muzzle→bullet (from the model — TE_TRACER was invisible). SLICEs: no pierce re-seed; speed approximated 6000.

WEAPON FIXES: (1) AKIMBO — AkimboFire now fires BOTH guns in AUTO mode (uzi_akimbo iFireModes 0x21→AUTO; the
viewmodel both-fire seq from the .mdl dump: uzi akimbo_shoot1=12, beretta/gold linkedshoot=26/7) not alternate
(fixed "switches hands"). (2) mp5sd (silenced-base) + mossberg fire sounds restored (were nullptr / wrong path
weapons/mossberg/fire.wav, neither precached). (3) ammo powerup MultiplyClips → clip = wi->iClip*2 (DOUBLE MAX,
user-directed) split across akimbo barrels. USER-DIRECTED DEVIATION: slow-pause self-slow 0.8→0.6 (= slow-mo,
so fire/move don't speed up; only bullets slower).

WEAPON-SECONDARY RE (the agent's scratchpad/WEAPON_SECONDARY_RE.md write didn't persist — findings here):
SecondaryAttack@0x717a8 dispatches firetype (grenade/knife/katana throw) → mode 4/5 (fire other gun) → per-
weapon: scope(attach&8)→ZOOM, else HOLD-POSITION toggle if wi+0x28c!=-1 (mk23/SOCOM=5, flips m_pActiveItem+0x2c
+ SendWeaponAnim). Akimbo fireBoth = bothButtonsHeld || mode==0(AUTO); barrel side = item+0x24; both uses
firelinked.sc. Fire-modes 0-5 = AUTO/SEMI/BURST/PUMP/AKIMBO_ALT/AKIMBO_BOTH at item+0x02, default from the
wi+0xc mask. SILENCER: client FUN_1003de90 picks <weapon>/fire-sil.wav when (iparam2&1); silencer = bit0 of the
attach mask (item+0x10 live / +0x124 stored); server docks dmg ×0.775. **Our fire event mis-packs barrel-side
in iparam2 — the original uses iparam2=attach-mask, bparam1=barrel side (align this when wiring #11/#15).**

OPEN (task tool #10-15 + sawed-off): #10 dblfire timeout (code looks right — need symptom/live test); #11
bought-silencer -sil sounds + event attach-mask align; #12 thrown knife/katana flying-speed slow; #13 shotgun
reload pose; #14 SOCOM hold-toggle (RE'd, not impl); #15 akimbo L/R-per-button (mode 4/5); sawed-off + maybe
other shotgun wavs are normal-recorded → need the 100*slow formula not 100+300*slow (ask user which weapons).
[[live-original-ts-debuggable]] [[match-original-names-for-coverage]]

## 2026-06-25 (cont. 69y) — kung-fu KICKS dispatch RE-faithful (CCombatAttack__6CTSGun @0x86078)

User picked the kung-fu KICKS dispatch from the carried weapon-feel TODOs. Rewrote `CTSGun::CloseCombatAttack`
(dlls/ts_gun.cpp) as a faithful port of the decompiled CCombatAttack__6CTSGun (scratchpad/ccombatattack.c),
replacing the old approximation. Static-RE'd, not guessed — decoded the raw decompile by computing the
entvars_t offsets from progdefs.h and pulling every DAT_ constant from the binary.

DECODE (entvars offsets): pev+0x180 = button (IN_ATTACK 1 / IN_DUCK 4 / IN_BACK 0x10 / IN_ATTACK2 0x800);
pev+0x1a5 = byte 1 of flags → &2 = FL_ONGROUND(0x200), &0x40 = FL_DUCKING(0x4000); pev+0x174 = view_ofs
(eye-height tweak on dive kicks); pev+0x250 = iuser4 (TS lock mask 0x50c30); player+0x79c = whiff-lunge
timer. Damage = local_114 (punches 0x23=35, kicks 0x2d=45, back-kick 0x37=55). Combo counters: punch
this+0x9fc (m_comboStep), kick this+0xa00 (m_kickCombo), last-anim this+0xa04 (m_lastMeleeAnim).

⚠️ REFIRE = local_108 (CORRECTED mid-session — see below): kung-fu is FULL-AUTO (standard ItemPostFrame holds
fire → repeats at the cap). I first read the refire as local_10c (0.1s normal punch) from the decompile's
frame arith, BUT that was WRONG — Ghidra's stack analysis was corrupted by a function-boundary overlap
(CCombatAttack tail mis-symbolizes as SlowMotionFlow; hand-disasm desyncs on the reordered blocks). The USER
live-confirmed the original punch cadence: full-auto capped at ~4/sec, identical hold-vs-click. 4/sec = 0.25s
= local_108 = DAT_00107e98 (the normal-punch constant) EXACTLY. So refire = local_108; local_10c (0.1-0.35) is
the VICTIM block-stun (BlockPlayer(player, local_10c*0.75)). [[live-original-ts-debuggable]],
[[flag-overriding-and-dead-code]].

KICK dispatch (IN_ATTACK2), first-match: duckkick (onGround && (IN_DUCK||FL_DUCKING), 45dmg, 1.0s) →
back-kick (movingBack: +back key when speed≤100, else v_forward·vel_unit ≤ −0.5; 55dmg, onGround?1.0:1.35s) →
sidekick-dive (airborne && speed>270 && IN_DUCK && fwdDot>0.5; 45dmg, 1.0s, resets kick combo) → ground kick
combo (onGround || speed≤200): 3 basic kicks (alt 0xb/0xc, 45dmg, 0.3s, m_kickCombo++) then a SPIN FINISHER
when m_kickCombo>2 (no-repeat cycle 0xa/0xe/0xd/0xf, 45dmg, 0.5s, reset) → else flying/jump kick (cycle
0x13/0x12/0xf, 45dmg, 0.75s, reset). PUNCH dispatch (IN_ATTACK), all 35dmg: airborne→air-punch (0.5s);
onGround && (m_comboStep>2 || duck-TAP[IN_DUCK && !FL_DUCKING])→uppercut (0.65s, reset); else normal jab
(0.25s, m_comboStep++, shown as 3 cycling v_melee jabs). Counters are PERSISTENT — reset only on a finisher/
dive, no time-window (removed the old 0.5s chaining reset). USER-CONFIRMED feel ("this looks good"): visible
3-jab→uppercut, full-auto 4/sec.

ANIM REMAP: the original kung-fu viewmodel has ~26 seqs (ids 0xa..0x19); our v_melee.mdl has 11, so each id
maps to the nearest seq + the 3rd-person player kung-fu seq. Dispatch/damage/timing are byte-faithful; the
anim is a remap (consistent with the cont.69q 11-seq decision).

SLICEs (documented in-code, not yet ported): the forward whiff-LUNGE (local_110 → player+0x79c, a decaying
PreThink velocity push applied on a MISS, scaled by GetMeleeMultiplier + doubled by the KungFu powerup) is a
separate PreThink subsystem (prethink.c lines 70/181) — kept as per-move magnitude comments; iuser4 lock-state
(0x50c30 → forced light attack) and player+0x814/0x815 bits (special-punch 0x17 / forced finisher) default to
"clear" (the common path). The melee screen-shake is unchanged (live-measured punch 20 / kick 25 → ×0.35).

Also recovered the canonical attack-id → 3rd-person body-anim names from HandleCloseCombat__12CTSAnimation
(0xa circfront / 0xb circkick / 0xc circkicklow / 0xd circback / 0xe latback / 0xf circspin / 0x10 sidekick /
0x11 backkick / 0x12 airkick / 0x13 airkick2 / 0x14 duckkick / 0x16 punchleft / 0x17 punchright) — so 0x10 is
SIDEKICK and 0x14 is DUCKKICK (my branch comments said dropkick/handstand; IDs/anims are what matter).

VERIFIED: hl.so + client.so build clean (glibc 2.14); deployed to the xash rig; runs no-crash; USER playtested
and confirmed "this looks good" (the full-auto 4/sec cadence + visible 3-jab→uppercut). See
[[live-original-ts-debuggable]], [[match-original-names-for-coverage]]. Ready to commit.

NEXT: remaining weapon-feel TODOs (dynamic-spread edge cases, weapon sway, coldcock whip, breakable respawn,
katana throw).

## 2026-06-25 (cont. 69x) — TS client PARTICLE / impact-effect subsystem DONE + LIVE-VERIFIED

The cont.69w next-task is COMPLETE. Byte-ported the original's custom client particle pool (NOT engine
tempents, NOT R_SparkShower) into **cl_dll/ts_particles.{h,cpp}**; hooked `TSParticles_Update()` into
`HUD_DrawTransparentTriangles` (cl_dll/tri.cpp); routed cl_dll/ev_ts.cpp `EV_TSBulletImpact`→`TS_ImpactEffect`
(per-surface dispatcher) and `EV_TSMelee`→`TS_MeleeImpact` (glint+sparks only). Full RE map persists in
scratchpad/impact-effects-re.md (on-disk, gitignored). Builds clean (glibc 2.14); runs in xash.

CRUX LOCATED (the handoff's stated blocker): the per-frame update/render = **FUN_100520d0(pool)**, found via
xrefs to the pool object DAT_1018bde8 from HUD_DrawTransparentTriangles. Reconstructed: manager
(FUN_1004ffe0/100509d0/100509a0/10050b60), billboard (FUN_10050ce0/10050080), 5 spawners (smoke 46680 /
debris 46bd0 / scintille 46f50 / spark-shower 492b0 / pellets 46a70), dispatcher FUN_10048f60 (per-surface
texture-char switch + 33/50/70 RandomLong gating).

VERIFIED BY LIVE GDB TRACE of the running original hl.exe (client.dll), NOT guessed — this caught what the
static decompile got wrong:
- pos += vel * frametime * _DAT_10079e88(**10**) — the missing ×10 velocity scale (sparks were too slow);
  gravity = gravity*0.1*sv_gravity(800) = ×80 in velocity-space (×10 in pos → real 800 u/s²).
- SMOKE = a CONIC FLARE: 4 puffs marching out pos=origin+normal*size*0.8*i (size ×0.8/step), each stretched
  (stretch 7) + growing (accel 0.1). The velocity divisor **DECREMENTS /(count-i)** (live vel.x = 3.5,4.4,
  13.0,9.9 across the 4 puffs) so the tip puffs eject faster — this is the "ejects further/faster" the user
  saw; I'd misread param_11 as a constant. frameRate = 20±10 jitter (live range 10-30). Every spawn arg +
  motion field (gravity/drag/bright/fade/accel/size/stretch/frameRate) byte-matched the live scratch particle.
- Spawner params RE-audited from raw ASM of FUN_10048f60 (the FP10-garbled decompile mis-mapped some): debris
  count = RandomLong(5/2,5) = 2-5 (the 0x20=32 is the velocity SPREAD, not count — I'd swapped them).

BUG FIXES found along the way: sprite-table name buffer 24→64 (truncated name broke the cache → 64-entry
table filled in seconds → all effects died PERMANENTLY); pool free-slot allocator OOB cursor walk; melee
event made SERVER-AUTHORITATIVE (flags 0, #ifndef CLIENT_DLL — the client-predicted melee world-trace misses,
so FEV_NOTHOST gave the local slasher no spark); glass penetration per-pierce **damage_adjust = damagemult
cvar value** (default 0, RE-matched — `*(damagemult+0xc)`), NOT 0.75 → two panes break only the first +
DECAL the second; the "black dots" were the stock Valve {shot decals from my added server DecalGunshot —
restored as server-side (entry+exit) so all clients see them, resolving the TS {shot from ts/decals.wad
(user: "bullet decals match TS exactly"). Debris chips now TUMBLE (orient-1 roll).

KNOWN APPROXIMATIONS (faithful behaviour, not byte-exact rendering): velocity-stretch billboard geometry
(FUN_10050080) reconstructed not byte-ported; debris roll done screen-space (orig uses hw.dll DAT_100d5b88);
concrete debris uses kRenderTransAlpha(4) not RE's TransColor(1) (our TriAPI draws INDEXALPHA TransColor as
an unmasked square). See [[sparta-overrides-write-weapon]], [[live-original-ts-debuggable]],
[[flag-unfound-re-early]].

NEXT: the carried-over weapon-feel TODOs (kung-fu KICKS dispatch, dynamic-spread edge cases, weapon sway,
coldcock whip, breakable respawn, katana throw) — see HANDOFF.md. Same live-trace-don't-guess method.

## 2026-06-24 (cont. 69w) — IMPACT-vs-surface effects: penetration+glass DONE; particle byte-port STARTED (THE next task)

User asked to validate/fix bullet·knife·thrown-knife·katana·thrown-katana·kungfu·grenade impact-vs-surface
logic + effects. Full RE + build plan in **scratchpad/impact-effects-re.md**.

DONE + DEPLOYED:
- BULLET PENETRATION rewritten to match the original CBasePlayer::ShootBulletsPlayer (@nm 0x5f69c; BULLETS.md
  had the wrong addr 0x6f69c=Ghidra). Uniform PointContents loop: penDepth=min(dmg*2,48); pass-through iff
  UTIL_PointContents(endpos+dir*penDepth)!=CONTENTS_SOLID. So pass-through depends on weapon damage + the
  enter→exit slice thickness (angle) — matches the user's spec. Removed our special glass branch.
  (dlls/ts_gun.cpp TSFireBulletsPenetrate.) damage_adjust per-pierce = 0.75 (VERIFY: orig reads damagemult[+0xc],
  relocated — pin live).
- GLASS no longer always breaks: PointContents tests WORLD brushes only, so glass (func_breakable ENTITY)
  always reads non-solid → bullets pass through; the pane SHATTERS only via ApplyMultiDamage iff dmg >= its
  health = **20** in TS maps (one 40). So most guns (>=20 dmg) one-shot it (correct), weak ones (skorpion 16,
  pdw 16, usas 7) don't.

THE NEXT TASK — BYTE-PORT the TS client PARTICLE subsystem (user: visual fidelity; it's CLIENT-LOCAL, does
NOT affect drop-in compat — that's protocol, already faithful). Impact effects feed a custom particle pool
(DAT_1018bde8), NOT engine tempents; our current EV_TSBulletImpact/EV_TSMelee use R_SparkShower = wrong (orig
sparks via sprites/scintille_metal.spr). Mapped: 300-byte particle struct (scratch DAT_1018a720), pool +
manager fns (FUN_1004ffe0/100509d0/100509a0/10050b60), spawner pattern, the dispatcher FUN_10048f60
(per-surface texture-char switch + RandomLong gating) + wrapper FUN_10044bf0 (5 call sites). STILL TO FIND:
the per-frame UPDATE+RENDER fn (the look). 6-step build plan + per-surface effect table + struct layout all
in scratchpad/impact-effects-re.md. Glass also wants a CRACK ("windshield") decal on non-breaking hits
(CBreakable::DamageDecal). NOT built — all builds clean+deployed so far; resume at the particle struct/manager.

## 2026-06-24 (cont. 69w) — ⭐ ANGLE-RECOIL fully RE'd + reconstructed (client-side aim kick)

Corrected the cont.69v trail (it pointed at vgui panel 0x648c0 / vtable[0x38] = a dead end; that slot
is just vgui::Panel::getAbsExtents on a plain Panel). Found the REAL system via live trace + static RE:

LIVE MODEL (unbuffered os.pread poll of cl.viewangles pitch @0x2d8aec4, user fired no-mouse): each shot
eases the pitch UP and STAYS (2s between shots = zero recovery → PERSISTENT aim move, NOT punchangle).
Per-frame deltas decay ×0.85, first frame ≈ 0.15× kick. Total ≈6.4° for the tested weapon.

SOURCE (client.dll, static RE confirmed): the "0.85/0.15" is frametime-dependent — real constant = 15.0.
- ON FIRE — FUN_10008230 @0x8230 (client twin of server VKickback@0x70414, SAME tuning): vert=amount*0.85;
  horiz=vert*firetypeScale (ft1=-0.25, ft4/5=+0.15, ft6/7=-0.30, ft8=+0.35, ft9=-0.35, else 0); *0.775
  duck; *0.66667 bullet-time. accum.pitch(DAT_10191d10)-=vert; accum.yaw(DAT_10191d14)+=horiz. amount =
  flViewKick(weapon 0x30)*modifier (call-site mods 0.35/1.0/2.0 — NOT yet pinned to weapons), firetype=0x348.
- EACH FRAME in V_CalcRefdef — FUN_10057ed0 @0x57ed0: remaining=target-applied; if |rem|²>=0.01:
  rate=min(frametime*15,1); delta=rem*rate; applied+=delta; viewangles+=delta (PERSISTENT). else snap to 0.
Full decompiles + constants (with file offsets) in scratchpad/recoil-live-findings.md.

RECONSTRUCTED (built clean, deployed to xash rig; awaiting playtest):
- NEW cl_dll/ts_recoil.h + TS_AddViewRecoil()/TS_UpdateViewRecoil() in cl_dll/view.cpp (g_recoilTarget/
  g_recoilApplied; ease at frametime*15, snap <0.1°; commits to the aim via Get/SetViewAngles each frame
  during the kick + bumps pparams->cl_viewangles for the current frame).
- TS_UpdateViewRecoil(pparams) called early in V_CalcNormalRefdef (before cl_viewangles→viewangles copy).
- EV_TSFire (cl_dll/ev_ts.cpp) local-player block now calls TS_AddViewRecoil(wi->flViewKick, wi->iFireType,
  args->ducking, false) — replaced the old V_PunchAxis(0,-2.0) placeholder. bullet-time arg = false (VERIFY:
  no client slow-mo flag wired). amount = flViewKick directly (modifier=1.0 assumed; CALIBRATE vs original).
NEXT: playtest vs the running original per weapon; if magnitude off, pin the FUN_10008230 call-site
modifier ([item+2]==1?1.0:0.35 @0x90be; bl?2.0:1.0 @0x9cea). Measure the YAW component live too.

UPDATE (same session) — playtest showed ours UNDER-DRIFTS vs original (skorpion full-auto). Root cause
found + fixed: the original recoil has a SECOND part — a per-shot damped SCREEN SHAKE (FUN_10057fa0 setup
@0x57fa0 + FUN_10058000 apply @0x58000, called from V_CalcRefdef) whose amplitude RAMPS WITH FIRE RATE
(amp=(0.15-dt)*15, cap 1, *0.775 duck; <=0.001 -> none). So single/slow shots = clean climb, full-auto =
climb + growing scatter (the "pronounced drift"). I'd only built the climb. Verified flViewKick[skorpion]
=0.25 is CORRECT from the binary (Write_skorpion @0x8d3f0), so the climb (0.21°/shot) is faithful — the
shake was the gap. Added the shake to cl_dll/view.cpp — but REVERTED it: the user clarified GUNS DON'T SHAKE, they just move
the aim (the climb); the screen-shake/"bounce-back" is the MELEE/KNIFE feel ("the knife doesn't recoil but
it does bounce back"). My ±1° shake on guns drowned the small climb -> the skorpion felt like "no functional
recoil." So view.cpp is now CLIMB-ONLY for guns (the working state for other weapons); the shake helpers were
removed (reconstruct FUN_10057fa0/58000 on the knife/melee path later if the bounce is wanted — full algo is
in scratchpad/recoil-live-findings.md). Debug Con_Printf removed. Built+deployed.

Skorpion specifics (RE-confirmed, no bug): single skorpion (firetype 9) routes FUN_100091e0 -> amount =
flViewKick directly (=0.25, verified Write_skorpion @0x8d3f0) -> climb 0.21°/shot — genuinely low, faithful.
Akimbo skorpion (firetype 4) routes FUN_100096f0 -> amount = 2.0*flViewKick + fires both barrels -> ~4x the
kick; user says the AKIMBO "looks more correct". So the single's weak climb matches the binary. OPEN: confirm
vs original single skorpion; if original single kicks harder, recheck the FUN_100091e0 amount path / weapon
0x38 gate. NEXT (separate): the KNIFE bounce-back = the shake on the melee path.

⭐ ROOT CAUSE of the weak/wrong recoil (cont.69w) — the Write_<weapon> values are an EARLIER init stage that
gets OVERWRITTEN on BOTH server and client by ONE authoritative combat table (NOT a server/client split):
- server: compile_weapons_info -> Sparta() @0x6fa78 -> SetSpartaWeapon(idx, spread, viewKick, recoil, dmg)
  @0x6fa34, sets s_weapon_info +0x28/+0x30/+0x34/+0x1c.
- client: FUN_10007a80 -> FUN_10007a50 — BYTE-IDENTICAL calls (same indices, same hex values).
So there's one real value per weapon. We had used the Write_ stage (the prev agent's "table verified from
Write_<weapon>" read the wrong stage). Found by tracing the single-skorpion recoil amount (live break = 0.5,
not the static 0.25; Sparta sets skorpion +0x30 = 0x3f000000 = 0.5). ~22 of 32 guns differ: skorpion 0.25→0.5,
beretta 3→2.5, uzi 1.5→1.75, benelli 10→13, m4 4→3.75, mp5k 3→2.25, desert 8→7.5, ak47 5→5.5, mossberg 14→8.5,
m16 4.5→4, ruger 0.3→0.35, bull 18→13, glock22 4.5→5, mac10 4→3, gold 5→5.5, contender/36 12→7.5, …
FIX: replaced kViewKick[] in dlls/ts_weapon_info.cpp with the Sparta/SetSpartaWeapon values (all 32 decoded;
the 6 melee/grenade indices Sparta doesn't list keep Write values). Built hl.so+client.so, deployed.
MELEE "bounce-back" shake ADDED (cont.69w) — RE-confirmed it's melee-only, NOT guns. Live-traced Shake_Setup
(client FUN_10057fa0 @0xc0c7fa0): swinging the knife fired it from FUN_1000a270 @0xa345 (the client melee
attack) with amp=4.5/dur=0.2/freq=10; firing GUNS did NOT fire it (the gun-recoil shake branch in FUN_10008230
@0x83b2 never tripped — confirms guns climb-only). Formula: amp = meleeWeapon flDamage * 0.15 (*0.66667 in
bullet-time) — knife dmg 30 -> 4.5 (matches). Shake = damped oscillation (random ±amp offset, sign-flip+halve
each 1/freq cycle, smoothed), TRANSIENT (settles to 0 = "shake + reset to where it was"), applied to the
RENDER view only (never the aim). Impl: cl_dll/view.cpp g_meleeShake + TS_TriggerMeleeShake(dmg,bt) +
TS_ApplyMeleeShake(pparams) (called after punchangle in V_CalcNormalRefdef, adds to pparams->viewangles);
triggered per swing in CTSGun::MeleeAttack (dlls/ts_gun.cpp, #ifdef CLIENT_DLL -> client-predicted = local
player; fire-rate gate = once per swing). Built hl.so+client.so (symbols: client.so has Trigger/ApplyMeleeShake,
hl.so has none — gating correct), deployed.

KUNG-FU shake ALSO added (cont.69w) — user reported knife bounced but kung-fu didn't (it's CCombatAttack,
not MeleeAttack). Live-traced: kung-fu punch/kick fire Shake_Setup from a DIFFERENT site (client FUN_1000af10
@0xb5e5) with amp = min(|velocity|*0.15385 [1/6.5, 0x1007a8f8], 45 [cap 0x1007a900]) * 0.35 [0x1007a8a8],
dur 0.2/freq 10. So it's VELOCITY-scaled (punch~7.0/kick~8.75 in the trace = the player's move speed), a
DIFFERENT multiplier (0.35) than the knife's fixed dmg*0.15. Generalized the trigger: TS_TriggerViewShake(amp)
(caller computes amp); knife passes flDamage*0.15, kung-fu passes the velocity formula. Wired in
CTSGun::CloseCombatAttack (#ifdef CLIENT_DLL, after SendWeaponAnim). CORRECTION (same session): user reported kung-fu only shook while MOVING — wrong, it must ALWAYS shake on
attack. The decompile's SQRT(velocity) is the attack's LUNGE velocity (the kung-fu move imparts a forward
lunge, non-zero even standing still), NOT the player's pre-attack movement — I'd wired m_pPlayer->pev->velocity
which is 0 when stationary. Our reconstruction doesn't model the lunge yet, so we approximate `power` with the
move's `dmg` (always > 0 -> always shakes): amp = min(dmg, 45) * 0.35. Built+deployed. AWAITING playtest:
knife/katana bounce (fixed dmg*0.15), kung-fu bounce ALWAYS on punch/kick, guns climb-only.

MAGNITUDE FIX (same session): user confirmed kung-fu bounce was too big (our dmg 35-55 vs original power ~20-25).
The shake `power` = the move's LUNGE velocity*0.15385 (we don't model the lunge), measured live as punch~20 /
kick~25 -> amp 7.0 / 8.75. Switched to per-category fixed power: kfPower = fSecondary ? 25 : 20; amp =
kfPower*0.35 (CloseCombatAttack, ts_gun.cpp). Built+deployed. VERIFY: special kicks (side/back/spin) + uppercut
not separately measured (assumed punch=20/kick=25 by the primary/secondary split).

FULL COMBAT TABLE re-sourced from Sparta (cont.69w, DONE) — Sparta also overrides spread(0x28)/recoil(0x34)/
DAMAGE(0x1c), so the Write_<weapon> values were the wrong stage for those too. Reconstructed the original's
structure in dlls/ts_weapon_info.cpp: added a SetSparta(idx, spread, viewKick, recoil, dmg) helper (mirrors
SetSpartaWeapon @0x6fa34) + 32 SetSparta(...) calls in the original Sparta() order, run after the Write_<weapon>
calls — overwriting flSpread/flViewKick/flRecoil/flDamage for the 32 firearms (6 melee/grenade/flag keep Write
values). Replaced the kViewKick array (now folded into Sparta); kept kUsesSpread. Decoded values from
ts_i386.so Sparta @0x6fa78 (full table in scratchpad/recoil-live-findings.md). Damage examples now corrected:
barrett 210->185, m4 34->33, beretta 29->27, desert 55->49, mk23 36->33. Built hl.so+client.so, deployed.

FULL WRITE_ + SPARTA AUDIT (cont.69w, both byte-exact). Extracted every Write_<weapon> field from ts_i386.so
(disasm of the 38 Write_ fns -> /tmp/write_fields) and diffed vs our table. All 32 guns' Write_ fields already
matched; fixed the melee/grenade outliers: m61 (clip 25->1, rate 0.092->1.0, spread 0.028->0.1, dmg/recoil->0,
pellets 1->2), knife pellets 1->5, sealknife 1->10, katana 1->0, flag firemodes 2->0. Added kWriteViewKick (the
Write_-stage view-kick, all 38) applied before SetSparta so the WRITE stage is byte-exact too (Sparta then
overrides the 32 guns; the 6 melee keep their Write_ vk, mostly unused). Verifier (parses our Write()+arrays+
kWriteViewKick+SetSparta vs binary Write_ AND Sparta): 0 Write mismatches, 0 Sparta mismatches, 32 SetSparta
calls. ONLY gap: flag (idx 29) is in the binary Write_flag but intentionally blank in our table (CTF objective,
not a fired weapon) — add Write_flag if full coverage of the non-combat flag entry is ever wanted.
AWAITING playtest. Recoil model = climb-only for guns (ease frametime*15, persistent); shake = melee/knife
(fixed dmg*0.15) + kung-fu (per-category power*0.35), transient render-only.

## 2026-06-24 (cont. 69v) — ⭐ LIVE TRACE located the angle-recoil (client view-prediction, NOT punchangle)

Static RE dead-ended on the aim recoil (no server v_angle write; CL_CreateMove + all 3 SetViewAngles
callers are standard mouse/keyboard). Resolved with a LIVE gdb WATCHPOINT on the running original (user
fired one shot, NO mouse): pitch kicked 1.61→1.12 (~0.49° UP) = the recoil moves cl.viewangles (the real
aim), confirming the user's "punchangle isn't a thing; recoil is angle-based."

ENTRY POINT (client.dll, live base this run = 0xc070000; PE imagebase 0x10000000 so live = base + RVA):
- cl.viewangles live @ 0x2d8aec4 (pitch)/0xc8/0xcc — found by reading engine GetViewAngles (ptr at
  gEngfuncs+0x88; gEngfuncs = Initialize's DAT_100d5b28 = RVA 0xd5b28) and decoding its mov-from-abs.
- The recoil write chain (bt): engine 0x1db74e0 (= gEngfuncs slot DAT_100d5c60) ← client FUN_1005b3c0
  (thunk, RVA 0x5b3c0, wraps that engine fn) ← client method @RVA 0x648c0.
- @0x648c0 is a per-frame client view-compute: it calls a VIRTUAL `this->vtable[0x38]`(&pitch,&yaw,&roll)
  to get the recoiled angles, then FUN_1005b3c0(&angles) commits them via the engine. **THE RECOIL MATH
  IS IN vtable[0x38]** of that client object (param_1; big object, fields 0x83/0x85-0x8a/0x2a9/0x2aa).
NEXT: decompile vtable[0x38] to extract the per-shot pitch/yaw recoil formula (scaled by weapon — small,
bigger on knives), then reconstruct it client-side. To find the vtable[0x38] target: read param_1's vtable
(*param_1) in the live process, slot 14 (0x38/4); param_1 is likely a client singleton.

WATCHPOINT RECIPE (reusable): gdb -p <pid> -batch with `watch *(float*)<cl.viewangles_pitch_addr>` (a
value-watch only fires on CHANGE — with no mouse, only the recoil changes it), `commands{ bt; detach; quit}`,
`continue`. Resolve bt return addrs to module+RVA: addr − module_base = RVA → rz-bin -E name. Module bases
via reloc-free prologue scan of /proc/<pid>/mem (CBreakable::Die for mp.dll, V_CalcRefdef for client.dll).

## 2026-06-24 (cont. 69v) — recoil view-punch (VKickback) + dynamic accuracy/spread WIRED + kungfu smoke

VKickback @0x70414 (recoil view-punch → pev->punchangle): vert = 0.85*flViewKick(0x30), horiz per firetype
(pistol +0.35, akimbo +0.15, pump -0.25, autoshotgun/sniper -0.30, auto-smg -0.35), *0.775 ducked, *0.667
bullet-time. Gated on weaponinfo+0x38 (all guns=1, melee/grenade=0). Called from StandardFire. NOTE the
original gates the SERVER apply on FL_FAKECLIENT (real players predict client-side) — we apply directly so
it shows on the listenserver. User clarified: punchangle is VISUAL ONLY (recovers, and bullets use v_angle
not punchangle) — the AIM-MOVE recoil is a SEPARATE client-side mechanism (viewangle kick on fire) NOT yet
located; sweep of the server (FireBulletsPlayer/DropKickback/PreThink) found NO v_angle write → it's in the
client CreateMove/input. TODO: RE the client-side aim recoil.

Dynamic accuracy/spread NOW WIRED (RE ScaleSpread @0x802a8 + the decay finally found in PreThink @0x808cc):
effective bullet spread = weapon flSpread(0x28) + per-player accumulator (CBasePlayer::m_flSpreadAccum =
player+0x778); ScaleSpread grows it per shot: accum = (1-recovery)*flRecoil(0x34)*factor + accum, capped 0.1,
factor=2.0 (*0.5 bullet-time, *0.775 duck), recovery = m_flSlowMotionGo*dt_since_last_shot (cap 0.5), duck
also tightens base *0.875; gated on bUsesSpread(0x2c). PreThink decays: accum -= m_flSlowMotionGo*frametime*0.1.
So spraying widens the cone, it recovers when you stop. SKIPPED edge cases: field_0xc8==0x20 (m60 5x recoil),
==0x12 (barrett max-spread-when-moving). Added m_flSpreadAccum (player.h) + m_flLastShotTime (ts_gun.h).

Kung-fu wall-hit smoke: added a server TE_SPRITE gun_smoke_add puff (smoke only, no decal/spark) in
CloseCombatAttack's surface branch — APPROXIMATION; user wants the original spawn (likely the client impact
dispatcher, same as bullets). TODO: RE the kung-fu client-event smoke spawn.

## 2026-06-24 (cont. 69v) — viewbob: removed eye-bob + RE'd the TS weapon-sway (figure-8 pendulum)

User: the stock HL bob dropped the EYES "like half-ducking" while running + snapped between 2 frames.
Root cause: stock `cl_dll/view.cpp` adds V_CalcBob to the MAIN view origin and the bob saturates its
-7..+4 clamp at TS speeds (square wave). RE of the ORIGINAL client.dll V_CalcRefdef (FUN_10067890):
the bob is applied to the VIEWMODEL ONLY (eyes get only a SMOOTHED view-height lerp), and the gun "bob"
is actually a custom **weapon-sway** (FUN_10066e70), not stock bob.

Fixes: (1) removed `pparams->vieworg[2] += bob` (eyes no longer bob — faithful). (2) replaced the stock
viewmodel vertical bob with the TS figure-8 sway. **Constants read LIVE from the running original**
(client.dll base via V_CalcRefdef reloc-free prologue; the FP consts are mostly DOUBLES): velmult=0.00333
(1/300), cap=1.1, phase wraps at **360**, accumscale=5.5, scale=100, ampvel=0.01, up-freq=0.0349 (≈2°),
right-freq=0.0174 (≈1°) → **2:1 ratio = figure-8 pendulum** (the user's "pendulum, not up/down"), upscale=0.11,
rightscale=0.3, fwdscale=0.65, ampscale=0.5, base=0.75, grip scales 0.1/0.3/0.5/0.7/1.0 (per weapon grip).
Phase += clamp(speed/300,1.1)·frametime·550, wrap 360; amp=speed·0.01; swayUp=sin(phase·0.0349)·amp,
swayRight=sin(phase·0.0174)·amp; viewmodel += right·swayRight·0.3 + up·swayUp·0.11.
SIMPLIFIED (not yet faithful): the amplitude SMOOTHING (state DAT_10191d6c/d78), per-weapon GRIP scaling,
and the forward base-position term (forward·(0.75−amp·0.5)) — the full FUN_100674a0 viewmodel-positioning.
Next: port the amplitude-smoothing + grip + forward term for byte-faithful sway.

## 2026-06-24 (cont. 69v) — knife slash + kung-fu combo + FULL weapon-table re-verification

**Melee anims/combos** (RE CCombatAttack @0x86078, KnifeFire @0x84044, verified vs model seq dumps):
- Knife `MeleeAttack`: removed the press-only gate → continuous slashing while held (gated by flFireRate like
  kung-fu); cycles 3 verified v_*.mdl slash seqs (knife {3 slash1,4 left,5 right}; katana/sealknife {2,3,4}).
- Kung-fu FIRE combo: replaced our mod-3+duck-uppercut with the real `field_0x9fc` counter — 3 jabs
  (punch_left 2/punch_right 3/punch_right2 10) then UPPERCUT (7), looping; +back forces it early.
- Lesson: the model sequence tables (v_melee 11 seqs, v_knife 8, v_katana 6) are the AUTHORITATIVE source
  for raw anim indices — the decompile's `a3` is NOT the SendWeaponAnim arg (it's a melee-fn param, a3=0xb
  exceeds v_melee's range). We ship the original's models, so model indices match by construction.

**Weapon table FULLY re-verified** (decompiled all 38 Write_<weapon>; `scripts/extract_weapons.py` is
UNRELIABLE — its `movl $imm,0xOFF(%reg)` regex grabs STACK temps colliding with struct offsets; e.g. the
knife "rate 0.07" was a `[esp+0x18]` store, real `weaponinfo+0x18`=0.4). Field map confirmed (0x04 clip,
0x08 caliber, 0x0c firemodes, 0x18 rate, 0x1c dmg, 0x24 pellets, 0x28 spread, 0x34 recoil, 0x30 view-kick,
0x2c uses-spread, 0xc8 akimbo, 0xcc single, 0x348 firetype). Fixed: knife (clip 18→1, dmg 40→30, **rate
0.07→0.4** — the user's "too fast" bug), katana (dmg 30→45, rate 0.4→0.7, spread 0.1→0.7), skorpion clip
40→20, gun firetypes m4/mp5k/mk23/desert/m16 0→6/9/8/8/6. firemodes/caliber/pellets verified already-correct.
TODO: rewrite extract_weapons.py to read the decompiles (authoritative) instead of the objdump heuristic.

**Dynamic accuracy system RE'd** (CTSGun::StandardFire @0x824a4 + ScaleSpread @0x802a8): per-shot spread
accumulates into CBasePlayer+0x778, effective spread = flSpread(0x28)+accum, grows by flRecoil(0x34)·factor
(factor 2.0, ×0.5 in bullet-time/DoubleFireRate, ×0.775 ducked), capped 0.1; VKickback applies view-punch
of flViewKick(0x30) per shot; 0x2c gates whether ScaleSpread runs. Added flViewKick+bUsesSpread to
s_weapon_info w/ verified values. NOT yet wired: the accumulator's RECOVERY/decay path isn't located yet
(0x778 only grows in ScaleSpread — decay must be in a per-frame path; flagged, not guessed). Next: find the
0x778 decay + RE VKickback's exact kick pattern, then implement the accumulation + view-kick in StandardFire.

## 2026-06-24 (cont. 69v) — knife→glass rework RE-VERIFIED vs ts_i386.so + 3 fidelity fixes

Decompiled the original `CTSKnife` (Spawn @0x6db54, WaitThink @0x6ea30, BounceTouch @0x6decc, WaitTouch
@0x6dce8, CreateKnife @0x6d9e4) + `KnifeFire` @0x74044 via the typed `re.sh` flow and checked the
uncommitted rework line-by-line. **The glass-break core is faithful — every readable constant matches the
binary**: WaitThink (think+0.1, speed>64, 64u velocity sweep, dont_ignore_monsters, flFraction<1, !IsPlayer
[vtbl +0xC8], health ≤ 0.1·speed, TraceAttack(knife_pev, 0.1·speed, dir·64, tr), ApplyMultiDamage(pev,pev));
BounceTouch (owner-ignore, per-type stab 150/100/50, player stab velocity=-0.25·vel + expire+10, worldspawn
speed≥100 && glance[−(normal·dir)<0.4 && normal.z≤0.98] → sparks+sound+velocity·0.5+return, else stick back
off dir·6, other-solid bounce -0.25·vel); Spawn (BOUNCE/BBOX, friction 0.8, gravity 0.85/0.75/0.3, expire+30);
ThrowKnife velocity = v_forward·**1900** (const @0x107f68 = 1900.0, NO player-vel term — confirmed in KnifeFire).

**3 fixes applied to dlls/ts_knife.cpp (build-clean, `T ts_knife` exported, glibc 2.14):**
1. `BounceTouch` now opens with the original's `if (pOther->pev->effects & EF_NODRAW) return;` (orig
   `(char)effects < 0`) — replacing an invented top-level `FClassnameIs("ts_knife")` self-ignore that was in
   the wrong place + skipped the NODRAW gate. 2. Knife-vs-knife ignore RELOCATED to the other-solid branch
   (where the orig's `classname=="knife"` check lives). 3. `StickAt` sets `angles.z = 90` (orig blade-roll).

**Known remaining divergences (intentional / low):** post-contact the orig switches SetTouch→WaitTouch +
gravity→0.75 (we substitute SettleAsPickup/StickAt pickup model); stick uses SOLID_TRIGGER+PickupTouch vs
orig SOLID_NOT+MOVETYPE_FLY; WaitThink dmg bits DMG_CLUB|DMG_NEVERGIB vs orig 0x8001000 (no glass-outcome
change); LINK name "ts_knife" vs orig "knife". **Tooling:** fixed `scripts/ghidra/re.sh` — its `sed` output
filter dropped every multi-line decompile body (only the log-prefixed first line survived); replaced with a
block-aware `_filter` awk helper, so `re.sh <Class>` now prints full typed bodies (how this verify was done).

## 2026-06-24 (cont. 69u) — the "auto-kick" = the JUMP-KICK (RE'd + implemented; user to playtest)

The cont.69t "proximity auto-kick" RE blocker is RESOLVED — and the premise was wrong. User
clarified (with screenshots: a mid-air flying-kick pose + a shattered window): **"Jumping with a
non-kungfu weapon while facing a breakable would kick it. If in kung fu it did NOT kick."** So it's
the **JUMP-KICK**, not the coldcock and not an automatic proximity trigger.

**RE debunked the previous agent's theory:** the "indirect function pointer @0x176858" is just the
normal **PLT/GOT JUMP_SLOT** for CloseCombatAttack — there ARE static callers. CloseCombatAttack
(@0x95f88) is reached from exactly two places: `CCombatAttack__6CTSGun` (bare-hands KUNG-FU, from
Primary/SecondaryAttack only when the active item ptr gun+0xcc==0) and **`CloseAttack__8CTSStunt`
(@0x9c630) = the jump-kick**. (`VKickback`/`DropKickback` are gun RECOIL, not leg kicks. The
coldcock — `AlternateAttack`→`ColdCock`→`ColdCockAttack` via button 0x40/IN_CANCEL — is the +alt2
pistol-whip = task #2, separate.)

**Jump-kick call path:** `CBasePlayer::PreThink` (orig 0x908cc) `if (button & IN_JUMP) Jump()` (same
as ours, player.cpp:2123) → `CBasePlayer::Jump` (orig 0x92864, vtable 0x158) tries
`CTSStunt::CloseAttack` FIRST (kick takes priority over the wall-flip/wheel acrobatics; basic jump
velocity is engine pmove). Called every think while IN_JUMP held → connects mid-flight.

**CTSStunt::CloseAttack RE (stunt obj @ player+0x4f0), all constants recovered** (⚠️ FILE OFFSET =
Ghidra addr − 0x10000; the FP constant pool reads 0 at the naive offset — the .so's DAT_001xxxxx
are .rodata at addr−0x10000; speed scale/thresholds are 8-byte DOUBLES the decompiler truncated):
- gates (return 0 = no kick): stunt-state timers, FL_FROZEN, **active weapon is the tsgun in
  kung-fu mode (id 0x10 && item+0xcc==0)** ← "no kick in kung fu".
- damage = clamp(|velocity| × 1/15, 10, 20) [scale 0.0666667 @0x109b10, lo 10.0 @0x107140, hi 20.0
  @0x107698, floor 10.0 @0x106da4, ceil 20.0 @0x107628]; ×2 if KungFu() power; reach 48.0 @0x106da8.
- hit resolver = CloseCombatAttack via CheckHull (@0x95e28 = TraceLine, then a **head_hull TraceHull
  fallback** = the "auto-connect when roughly facing it" feel); knockback only if damage > 30.0
  @0x107600 (victim shoved at kicker speed + velocity.z=235); EMIT player/closecombat.wav.
- 3rd-person body anim = RANDOM_LONG(0,2)→ids {0:10,1:14,2:13} → HandleCloseCombat (@0x6d8d0) maps
  10/13/14 → **circfront / circback / latback** (player-model kick seqs). Cooldown 0.7s.

**Implemented** (`dlls/ts_gun.cpp` `TSTryJumpKick(CBasePlayer*)`, `#ifndef CLIENT_DLL`-guarded;
decl `dlls/ts_weapons.h`; cooldown `m_flNextJumpKick` in `player.h`; hooked at the top of
`CBasePlayer::Jump` in `dlls/player.cpp` BEFORE the pogo gate so it runs every think). All constants
RE-grounded. Uses GetPlayerTSGun/KungFu/GetGunPosition/m_TSAnim.PlayKungFu(circfront/circback/latback).
**Built clean** server + client (glibc 2.14); TSTryJumpKick in hl.so, correctly ABSENT from client.so
(guarded — it'd otherwise pull the server-only KungFu/UTIL_TraceHull into the prediction build).
Installed to the xash root. **NOT yet playtested — handed to the user to test** (their preference).
Not committed yet (pending playtest).

**Queue still open:** task #2 coldcock viewmodel whip (LAB_10043800/coldcock.sc), #3 predicted
empty/pump/closecombat effects, + katana right-click (KatanaThrow) noted in 69t.

### cont.69u (later) — bullet/breakable/slow-mo/sound RE batch → reference/BULLETS.md (no impl yet)
User opened a new front (bullet penetration, glass shatter, slow-mo bullets, sound pitch). RE'd +
documented in **reference/BULLETS.md**. Key confirmed (LIVE, not dead-code — user confirmed the
bullet behavior is correct):
- **Live fire path = `ShootBulletsPlayer`@0x6f69c** (from StandardFire/ShotgunFire/AkimboFire), NOT
  stock FireBulletsPlayer (= our current gap). It **recurses for PENETRATION**: after a hit, step
  `min(dmg×2, 48)` forward; if `PointContents` past != SOLID (surface thinner than that) → recurse
  (mode 2) to hit the solid beyond. So firing glass breaks it AND hits the wall behind. Dual
  enter+exit decals = TODO (pin the decal sites).
- **Glass shatter** = `Die__10CBreakable`@0x752e0 material switch: case 0 GLASS →
  **gmsgTSCreateDebris** (client SPRITE shower); all others → **TE_BREAKMODEL** (gib MODEL). Exactly
  the user's spec (glass=sprites, wood=model). Shatter dir = attack dir; init vel = this+0x110 ×
  material mult. Client gmsgTSCreateDebris handler + glass sprite + base velocity = TODO RE.
- **Slow-mo bullets-as-models** = `CBulletManager` (rbullet gate: TSGameRules+0x578 slow-PAUSE or
  slow player → tracked-model bullets; else instant hitscan). Both live.
- **Sound pitch** (weapons recorded slow → played fast): fire sound is client-side (FUN_1003dfa0);
  pitch presumably from the slow-mo factor. Mechanism NOT yet pinned — flagged as TODO.
Saved two memories: `ghidra-rodata-const-offset` (file=Ghidra−0x10000, doubles) and
`flag-overriding-and-dead-code` (the user's live-vs-dead RE preference, buy-menu lesson).

### cont.69u (later 2) — ✅ GLASS SPRITE SHATTER implemented (built, awaiting playtest)
RE'd the full glass-shatter pipeline + implemented it:
- **Server** (dlls/func_break.cpp): `CBreakable::Die` for `matGlass` now sends a new `gmsgTSCreateDebris`
  (origin, pev->size, material byte 0, **g_vecAttackDir** = the bullet/attack dir, refpoint, glassfrag
  sprite index, velMag) INSTEAD of TE_BREAKMODEL; other materials keep TE_BREAKMODEL (gib model).
  Precaches `sprites/glassfrag.spr`. (Matches Die@0x752e0 material switch: case 0 glass→message.)
  ⚠️ velMag=150 is a FLAGGED placeholder — the per-pane base (CBreakable+0x110) is a runtime/keyvalue
  value not recovered statically; tune from playtest (user said "discover the init velocity").
- **Client** (cl_dll/ts_debris.{h,cpp}, CHudTSDebris registered in hud.{h,cpp}, added to CMakeLists):
  MsgFunc_TSCreateDebris reads the payload + reconstructs FUN_10047cb0 — a **9×9=81-fragment grid** of
  glassfrag.spr `R_TempSprite`s across the pane FACE (drops the smallest dim = pane normal; centre 0.5,
  step 1/9), each flying in the shatter dir ×velMag with ±15 spread, gravity, FTENT_FADEOUT|GRAVITY|
  COLLIDEWORLD|ROTATE, life RANDOM(2,5)s. Constants RE'd from client.dll (file=VA−0x10000000).
  Both DLLs build clean (glibc 2.14); glassfrag.spr present in the xash ts root; installed.
  **Blend-mode fix (user playtest):** glassfrag.spr is **SPR_ADDITIVE** (texFormat 1) → must use
  **kRenderTransAdd** (black keys out); kRenderTransAlpha showed the black bg. Also the scale was way
  too big (pane-sum/18 → clamped 1.5) → now pane-sum/180 clamped [0.08,0.3] = little shards. General
  rule: match the temp-sprite rendermode to the .spr texFormat (ADDITIVE→TransAdd, ALPHTEST/INDEXALPHA→
  TransAlpha). woodfrag=ALPHTEST + debris_concrete3=INDEXALPHA impact chips already on kRenderTransAlpha.
  **Playtest round 2 fixes (glass + chips):** (1) the .spr files have MULTIPLE FRAMES = different shard
  SHAPES (glassfrag 9, woodfrag 7, debris_concrete3 5); the original picks a RANDOM frame per fragment —
  mine all used frame 0 (identical shapes). Now set `t->entity.curstate.frame = RANDOM(0,nframes-1)`.
  (2) spin rate too low → bumped baseline.angles[2] to ±500°/s (was ±120). (3) velocity too directional
  (all flew one way → clumped on landing) → forward-bias along dir ×0.4 + LARGE per-axis random scatter
  (±velMag*0.6). (4) **direction was BACKWARDS** — glass flew toward the shooter; g_vecAttackDir points
  breakable→attacker (the stock gib code uses -g_vecAttackDir), so the glass message now negates it
  (flies away, the bullet travel dir). Sprite rotation gotcha: xash rolls type-2 (SPR_FWD_PARALLEL)
  sprites only when angles[ROLL]≠0 (gl_sprite.c promotes to _ORIENTED) — so the initial roll must be set.
### cont.69u (later 3) — ✅ BULLET-IMPACT FALLING FRAGMENTS implemented (built, awaiting playtest)
User: "bullet hits spawn sparks + smoke + little fragments that drop to the floor." Smoke+sparks were
already in EV_TSBulletImpact (cont.69t); added the falling chips. RE'd the client impact dispatcher
FUN_10048f60 (switch on texture char) → FUN_10046bd0 (the fragment temp-ent spawner): velocity =
surface-normal×16 + RANDOM(±32) spread, gravity, rotation, ~2-5 chips, short life. Per-material:
**wood/dirt → woodfrag.spr, concrete/default → debris_concrete3.spr**; metal/grate/vent/glass → sparks
(scintille_metal, no chips); flesh/water → none (server blood). Impl: cl_dll/ev_ts.cpp EV_TSBulletImpact
spawns count=RANDOM(2,5) R_TempSprite chips (FTENT_FADEOUT|GRAVITY|COLLIDEWORLD|ROTATE, life 0.8-1.4s) in
the material switch; precached woodfrag/debris_concrete3 server-side (dlls/ts_weapons.cpp). Both build
clean, assets present, installed.

### cont.69u (later 4) — ✅ BULLET PENETRATION + DIVE-THROUGH-GLASS implemented (built, awaiting playtest)
- **Penetration** (dlls/ts_gun.cpp `TSFireBulletsPenetrate`, #ifndef CLIENT_DLL; called from StandardFire
  + AkimboFire server-side, client keeps the prediction FireBulletsPlayer): per pellet, player-synced
  spread then a pierce loop (max 4) — TraceAttack+texture-sound+ENTRY DecalGunshot, then if the hit is a
  brush/world and dmg>5, step `min(dmg*2,48)` forward; if PointContents there ≠ SOLID (thin slice OR an
  entity brush like glass), decal the EXIT/back face + continue the trace (dmg ×0.75). So fire at glass →
  glass breaks AND the round hits the wall behind, bullet-holes on both faces. (orig ShootBulletsPlayer
  @0x6f69c pierce recursion.) Penetration gated to brush surfaces (not players).
- **Dive-through-glass** (dlls/ts_gun.cpp `TSCheckRunThrough`, #ifndef CLIENT_DLL; called every
  CBasePlayer::PreThink): speed≥64 → hull-trace 64u forward ALONG THE VELOCITY direction → TraceAttack the
  hit (damage = speed, ×0.1 grounded vs full airborne) → breaks glass / shoves people, NO kick anim.
  Last-hit guard via new EHANDLE m_hRunThroughLast (player.h). (orig CTSStunt::CheckForBreakables@0x9bd9c.)
- Both build clean (server+client); symbols in hl.so, absent from client.so (guarded); installed.
### cont.69u (later 5) — penetration/knife glass-CHAIN + knife steep-angle (RE'd) + impact-chip polish
- **Bullet chain through multiple glass** (dlls/ts_gun.cpp TSFireBulletsPenetrate): the continuation
  jumped the full pierce-depth (min(dmg*2,48)) past the hit → OVERSHOT a 2nd pane within 48u. Fixed: if
  the surface SHATTERED (pHit->pev->solid==SOLID_NOT after ApplyMultiDamage), continue from just past the
  ENTRY (+1u) so a close next pane is still hit; only jump past the back face for still-solid walls. Cap
  raised 4→8. Whole glass chain now breaks.
- **Knife glass-CHAIN** (dlls/ts_knife.cpp KnifeTouch): was "break 1st pane → trace forward ignore_glass →
  stick in the wall beyond" (skipped every pane past the 1st). Now on a breakable it CAN break, the knife
  KEEPS FLYING (velocity ×0.9/pane); the broken pane is SOLID_NOT so it won't re-touch it → it hits + breaks
  the next pane, chaining until a non-breakable wall.
- **Knife steep-angle no-stick** (RE CTSKnife::BounceTouch@0x7decc): a glancing wall hit DEFLECTS instead of
  sticking — bounce when `-(normal·flyDir) < 0.4` AND `normal.z <= 0.98` (not a flat floor); else stick.
  Constants RE'd (doubles): align 0.4 @0x107ed8, normal.z 0.98 @0x107ed0, speed thresh 100, glance ÷2,
  embed 6. Uses tr.vecPlaneNormal (SDK TraceResult has it; cf. gauss.cpp).
- **Impact chips** (cl_dll/ev_ts.cpp): count 1-3, per-chip random eject speed 120-300 along the normal +
  ±130 spread, kRenderTransAdd (additive=bright; rendercolor only MODULATES so couldn't brighten), no
  FTENT_FADEOUT (abrupt 1s cutoff), random frame (woodfrag 7/concrete 5 shapes), spin ±500.
- **Glass shards** (cl_dll/ts_debris.cpp): random frame (9 shapes — was all frame 0), spin ±500, velocity
  forward-bias ×0.4 + ±velMag*0.6 scatter, dir NEGATED (g_vecAttackDir points at shooter), small scale.

### cont.69u (later 6) — knife glass health/chain/angle + dive-barge airborne gate (RE notes)
- **Walking-breaks-glass FIXED (RE-confirmed):** CheckForBreakables@0x9bd9c line ~46 returns early when
  `pev->flags & FL_ONGROUND` (and the +0x79c stunt flag is 0) → the original ONLY barges through glass
  when AIRBORNE. Our TSCheckRunThrough was missing that gate; added `if (FL_ONGROUND) return;`.
- **Knife glass (dlls/ts_knife.cpp KnifeTouch rewritten as a unified per-surface trace-loop):** ALWAYS
  TraceAttack a breakable it hits → damage ACCUMULATES (weak glass breaks after a few throws; strong/
  high-health glass survives any number — standard CBreakable health). Head-on break → punch through to
  the next pane (whole chain breaks); survived OR glancing → Deflect (reflect+½ speed, keeps the damage).
  Walls stick head-on / deflect glancing.
- **RE flag (faithfulness):** the steep-angle thresholds ARE RE-grounded — BounceTouch@0x7decc bounces
  when `-(normal·flyDir) < 0.4` AND `normal.z <= 0.98` (consts: 0.4 @0x107ed8 dbl, 0.98 @0x107ed0 dbl,
  speed 100, ÷2, embed 6). BUT in the binary that check sits in the **worldspawn (solid wall)** branch;
  BounceTouch's damage path is gated on **IsPlayer (vtable+0xc8)** so only PLAYERS get knife-TraceAttacked
  there — breakables take a separate embed path. So how the original knife damages/chains/angle-deflects
  GLASS isn't fully resolved statically (likely the glass's own engine break-on-touch + physics). I
  implemented the user's OBSERVED behavior using the RE-grounded thresholds + standard CBreakable health;
  applying the angle check to breakables is an extrapolation, flagged to the user.

### cont.69u (later 8) — ⭐ LIVE gdb trace found the knife→glass break path (mechanism = THINK, not touch)
The user is running the ORIGINAL TS in hl.exe under Proton (PID 3844527; mp.dll loaded, Wine maps PE by
RVA — effective base 0x1e13ffb0, i.e. CBreakable::Die RVA 0x4b9a0 → live 0x1e18b950). Attached gdb,
HARDWARE-bp on Die, user threw a knife at glass, captured the stack. **Backtrace (definitive):**
`GetNewDLLFunctions(engine think dispatch) → CTSKnife::WaitThink → [swept-move fn] → CBreakable::TakeDamage
→ CBreakable::Die`. So the thrown knife breaks glass through its **per-frame physics THINK (a swept move
that TakeDamages breakables it crosses)** — NOT through BounceTouch (the touch), and NOT BreakTouch (which
IS IsPlayer-gated, verified: CBreakable vtable+0xc8 reloc → IsPlayer). Our reconstruction breaks glass in
KnifeTouch — the WRONG MECHANISM, which is why it was inconsistent/angle-wrong. RestorePhysics/SavePhysics
(vtable 0x118/0x11c, called by WaitThink) are only slow-mo velocity scaling; the swept-damage fn (the Windows
frame between WaitThink and TakeDamage, unexported) isn't cleanly Linux-mapped yet. **Next:** either rework
the knife to a think-based swept-damage, OR a 2nd live trace (bp on CBreakable::TakeDamage, read the damage
arg + dir) to confirm the exact damage/angle formula. gdb how-to saved in [[flag-unfound-re-early]].
**Method memories saved:** `flag-unfound-re-early` (don't make shit up vs the RE; dynamic gdb trace recipe).

### cont.69u (later 9) — ✅ knife→glass reworked to the ACTUAL implementation (WaitThink swept-break)
Found `WaitThink__8CTSKnife` @0x6ea30 (it WAS in ts_i386.so; I'd missed the grep — the live trace + mp.dll
export name `CTSKnife::WaitThink` pointed right at it). The real rule (RE'd, all constants confirmed):
the knife's in-flight THINK runs every **0.1s** (DAT_00107138), and if speed > **64** (DAT_00107ea0) it
sweeps a **64u forward trace along the velocity**; if it hits a NON-player breakable whose
`health <= 0.1*speed` (DAT_00107138 * speed) it `TraceAttack`s `0.1*speed` (DMG 0x8001000) and breaks it.
**NO angle scaling, NO accumulation** — either the throw kills the pane in one hit or nothing happens.
Reworked dlls/ts_knife.cpp: added `CTSKnife::WaitThink` (the sweep) + `SweepBreak` (the shared one-hit
rule), Spawn now SetThink(WaitThink)@0.1s + a 15s expire; KnifeTouch simplified — stab flesh / break-and-
fly-through if SweepBreak succeeds / **bounce off (0.25*-vel) if too strong** / stick-or-deflect walls.
Removed the invented angle-scaled + accumulation + energy-falloff chain. Built clean, installed.
**Docs:** added CLAUDE.md **§4b** (live gdb trace + mp.dll cross-ref method) next to the Ghidra flow;
enriched the `flag-unfound-re-early` memory. mp.dll Ghidra project at recovered/ghidra/mpdll.
**TODO (user): if this plays right, audit docs/ledger for other "couldn't find / made-shit-up" spots and
re-RE them via the mp.dll/live-trace method.**

### cont.69u (later 7) — knife↔glass behavior reworked to the user's observation (RE caveat noted)
RE'd the glass-break paths fully: **CBreakable::BreakTouch@0xc0c44** (velocity break-on-touch) is gated on
`pOther->IsPlayer()`, and **CTSKnife::BounceTouch@0x7decc**'s only TraceAttack/damage path is ALSO gated on
IsPlayer — so a thrown knife never breaks glass via either in the binary (breakable → bounce-off + fall;
ShouldCollide just returns 1). The knife→glass BREAK path is NOT statically locatable. User corrected: knives
break glass head-on OR **fall straight down if they can't** — they NEVER stick in glass; and they usually
break ONE pane then **drop at the second** (energy loss per pane). Implemented (dlls/ts_knife.cpp KnifeTouch):
ANGLE-SCALED damage `m_flDamage * -(normal·flyDir)` (head-on full → breaks weak glass; glancing chips;
accumulates), break → punch through to the next surface with **dmg ×0.5 per pane** (so it rarely chains
past ~two), survive → **fall straight down** (kill horizontal velocity + back off the pane; lands as a floor
pickup). Walls keep the RE-grounded stick-head-on / deflect-glancing (0.4/0.98). Flagged: the knife-breaks-
glass mechanic itself isn't byte-faithful (couldn't locate it) — modeled on the observation + RE'd angle/health.

- ⚠️ Remaining queue: **sound pitch** (recorded-slow/played-fast) still needs the focused client decompile
  (FUN_1003dfa0 sound-play pitch vs the slow-mo factor) — flagged, not pinned. Plus #2 coldcock viewmodel
  whip, #3 predicted empty/pump/closecombat, katana right-click throw.

- **glass break paths (RE'd):** (1) shoot it → CBulletManager/ShootBulletsPlayer TraceAttack (penetration
  also lets the bullet continue to the wall behind); (2) jump NEXT to it → the jump-kick (CloseAttack,
  kick anim); (3) **dive/run INTO it fast → CTSStunt::CheckForBreakables@0x9bd9c** (per-think from
  PreThink: speed≥64 → velocity-direction hull trace 64u → TraceAttack damage=speed; no kick anim =
  "jump before reaching it, shatter, dive through"). #3 documented in reference/BULLETS.md, NOT yet impl
  (new task #6).

---

## 2026-06-23 (cont. 69s) — .sc client weapon-param subsystem: steps 1+2 (table shared + deploy cache)

Started the FULL client weapon-param subsystem (the cont.69r NEXT TASK). Steps 1 & 2 done +
in-engine verified. Committed as **fa44c26** (one commit for both steps — ev_ts.cpp +
cl_dll/CMakeLists.txt straddle both, so a clean split would need non-buildable intermediates).
NOTE: committed with `--no-gpg-sign` (user authorized "just commit skipping gpg" after the
gpg-agent passphrase cache expired mid-session + pinentry timed out headless). Re-sign later if
the unsigned commit matters.

**Step 1 — shared s_weapon_info table into the client.** Split the pure weapon DATA layer
(tsweapon_t, s_weapon_info, get_weapon_info, the per-caliber ammo/fire-mode helpers) out of
dlls/ts_weapons.{h,cpp} into a new SHARED TU dlls/ts_weapon_info.{h,cpp}, built into BOTH hl.so
and client.so (the original keeps a client copy at DAT_100aa050 via a client compile_weapons_info
mirror — we share one TU). ts_weapons.{h,cpp} now hold only the server-only PrecacheAllTSWeapons.
ev_ts.cpp muzzle-grip reads get_weapon_info()->pszAnimExt (was a duplicated local array). Verified:
both build (glibc 2.14), get_weapon_info linked into client.so, M4 fires + flare draws in-engine.

**Step 2 — the active-weapon param cache (client) + draw event.** RE'd the handler architecture
(client.dll Ghidra): each handler caches its args at entry (FUN_100425b0 draw / FUN_10042690 reload:
type=iparam1, info=get_weapon_info(type), barrelMask=iparam2, entindex=args+4, origin/angles/vel);
the fire handler FUN_1003de40 dispatches on the fire-type byte weapon_info+0xd2 (==our iFireType:
1=pump, 4/5=akimbo, else standard) — so the WEAPON TYPE TRAVELS IN THE EVENT. The ~70 DAT_10189xxx
globals are the client's INTERNAL prediction state, NOT the protocol. **The contract = event NAME +
iparam/fparam/bparam packing** (reload also packs fparam1=stage 0/9/10). Per CLAUDE.md (behaviour,
not byte) the cache is a clean modern struct, not a 70-global byte-port. Full RE in reference/EVENTS.md
"cont.69s".
- cl_dll/ts_weapon_predict.{h,cpp}: `struct TSActiveWeapon {type, info, barrelMask, entindex,
  origin/angles/velocity, isLocal; UpdateFromEvent(args);}` + global g_tsActive + EV_TSDraw (draw.sc
  handler = seeds the cache). EV_TSFire now refreshes g_tsActive too. draw.sc hooked in hl_events.cpp.
- Server (ts_gun.cpp): m_usDraw precached; Deploy() plays draw.sc ADDITIVELY (iparam1=type,
  iparam2=akimbo barrel mask 2/4/6 else 1) — keeps the server DefaultDeploy visuals, only primes
  client state. Verified in-engine: deploys (knife/M4) fire the draw event, NO crash, no bad-event
  errors, fire path unchanged.

**Step 3 — ⚠️ anim-prediction BLOCKED (in-engine proven), decision needed.** Tried porting reload
as a predicted event (EV_TSReload → EV_WeaponAnimation, replacing the server SendWeaponAnim).
In-engine: the clip refilled (server reload ran) but the reload viewmodel anim NEVER played across
~1.8s. Root cause: our CTSGun is server-authoritative + not registered in the client weapon-
prediction loop, and CLIENT_WEAPONS is defined, so HUD_WeaponsPostThink overwrites any predicted
viewmodel anim every frame. (The fire event's local effects DO work, so EV_IsLocal is fine — it's
specifically EV_WeaponAnimation that can't stick. Matches the long-standing EV_TSFire comment.)
**Experiment reverted** (reload back to SendWeaponAnim, no regression); finding documented in
reference/EVENTS.md "cont.69s — anim-prediction BLOCKED" + a code note at CTSGun::Reload.

**The real fork (user decision):**
- (A) Port only the SOUND/EFFECT half of each event (drop-in arg packing + sprites/shells/punch/
  temp-entities), anims stay SendWeaponAnim. Low value — EV_TSFire already covers the main effect
  case and SendWeaponAnim already triggers the models' embedded reload/clip sound events.
- (B) Make CTSGun CLIENT-PREDICTED first (a client weapon object in HUD_InitClientWeapons /
  HUD_WeaponsPostThink / HUD_GetWeaponData, mirroring CTSGun clip/timers/state) — the real gate for
  faithful predicted anims, and a large separate effort. Then the 11 handlers predict properly.
- (C) Stop at the foundation (steps 1+2 shipped); revisit prediction later.

Steps 1+2 (committed 1cf2499) stand regardless — the shared table + the active-weapon cache are
prerequisites for (B) too.

**User chose (B) — make CTSGun client-predicted.** Progress:
- **3a DONE (committed bb0594f)** — dlls/ts_gun.cpp now compiles INTO client.so (added to
  cl_dll/CMakeLists, like the HL weapon .cpp files). It compiled unchanged; only failed to LINK
  vs server-only symbols (TSCreateWorldGun/AmmoPack/ThrowM61, CTSAnimation::Animate/PlayKungFu,
  PrecacheAllTSWeapons, DoubleFireRate, DispatchSpawn/Touch) → new cl_dll/ts_gun_stubs.cpp gives
  client no-op stubs (server-authoritative side-effects the prediction must skip; none on the
  hot path). ZERO edits to ts_gun.cpp. 35 CTSGun methods now in client.so. Infra only — nothing
  runs the client CTSGun yet; client loads+spawns+fires fine.
- **3b/3c SCAFFOLDED (committed 98fa78e), DISABLED pending debug.** Built the whole harness:
  extracted the CTSGun class to dlls/ts_gun.h; added CTSGun::GetWeaponData/SetWeaponData (ship
  weapon TYPE=iuser1, active-caliber RESERVE=iuser2, fire-mode=iuser3; clip/timers/m_fInReload ship
  generically); added the client g_TSGun + HUD_PrepEntity + the HUD_WeaponsPostThink TSGUN_WEAPON_ID
  case in cl_dll/hl/hl_weapons.cpp. UseDecrement() gates it.
  - **Enabled it and tested in-engine — NOT correct, so left DISABLED (UseDecrement=false +
    registration COMMENTED OUT; runtime verified unchanged / no regression):** prediction RAN
    (ItemPostFrame client-side) but (a) the predicted viewmodel anim STILL didn't stick (fire +
    reload stayed in the idle pose — reload frames pixel-identical), and (b) fire effects DOUBLED
    (muzzle/shells, no event dedup). No crash (EMIT_SOUND = client no-op via hl_baseentity.cpp;
    MESSAGE_BEGIN only in UpdateClientData/SendWeaponStatus, NOT on the prediction path).
  - **To re-enable:** flip UseDecrement→true + uncomment the 2 registration lines in hl_weapons.cpp.
  - **ROOT CAUSE FOUND (Con_Printf-instrumented HUD_WeaponsPostThink, then reverted).** 3b/3c are
    actually CORRECT: ItemPostFrame runs client-side for id 16, gates pass, SetWeaponData mirrors
    right (wpnType=5/clip/reserve). The bug is a **TIME-BASE mismatch**: the engine decrements a
    UseDecrement weapon's m_flNextPrimaryAttack as a RELATIVE countdown each client frame (observed
    37.92→37.85 while client time rose), but CTSGun uses ABSOLUTE gpGlobals->time → timers corrupt →
    predicted attack gate misfires → SendWeaponAnim never driven (animBefore==animAfter==0) → idle
    viewmodel = "anim doesn't stick". CTSGun uses absolute time because **CLIENT_WEAPONS is defined
    GLOBALLY** (CMakeLists, both hl+client targets) making UTIL_WeaponTimeBase()==0 on the server too
    — cont.69q's gpGlobals->time was the workaround, which is incompatible with prediction.
  - **✅ FIXED + ENABLED (committed ee642ab) — CTSGun is now CLIENT-PREDICTED, anims stick.** The fix
    was NOT to rescope CLIENT_WEAPONS (it must stay global — the server needs it for GetWeaponData +
    the CBasePlayer::ItemPostFrame decrement loop @player.cpp:2823, gated `#if CLIENT_WEAPONS` +
    `gun->UseDecrement()`). The real fix: revert CTSGun's 34 timer sites gpGlobals->time →
    UTIL_WeaponTimeBase() (0-base; the 4 real-time deltas m_lastCCTime/m_blockBar keep gpGlobals->time).
    With UseDecrement()==true the server decrement loop now decrements CTSGun's 0-base timers (cont.69q's
    absolute-time workaround was only needed because UseDecrement was false). Re-enabled UseDecrement +
    the g_TSGun registration. Fire/akimbo events → FEV_NOTHOST so the predicting host plays its own
    predicted muzzle/shell (no double).
  - **Verified in-engine** (Con_Printf, removed): `cl: StandardFire`/`cl: Reload` run client-side;
    predicted clip decrements; fire (slide) + reload viewmodel anims ANIMATE (were static); SINGLE
    muzzle (dedup works); server fires repeatedly (no fire-once-stuck). No crash. Both DLLs build.
  - **TEST-HARNESS NOTE for next agent:** the buy/select is flaky under blind xdotool — `tkw`/`weapon_N`
    didn't reliably wield the M4 (random loadout auto-selects the highest-priority owned weapon, often a
    knife). What worked: spawn (click through the gate 3-4×), then `drop` ×2 in console to shed the
    melee and auto-select an owned GUN. Then fire (MOUSE1=+attack; note ENTER is ALSO +attack).
  - **✅ DEDICATED-SERVER firing pass DONE (no regression).** Ran the ReHLDS rig (metamod+rcbot,
    `~/Containers/hlds/hlds-linux64-test`, ts_wyndham, bots) with a TEMP bot-give hook (M4 + ammo +
    `m_iClip=2` in TSDeployArsenal) + TEMP server `[TSSRV]` fire/reload probes — pure server-side, NO
    client prediction. Result: bots fire the M4 in sustained full-auto (clip 29→3, 70 events) AND the
    full empty→reload→refill cycle works (`fire clip=0 → reload → fire clip=29 → fire 28…`, 52 fire/5
    reload). No fire-once-stuck, no crash. So the UTIL_WeaponTimeBase()+UseDecrement 0-base holds on a
    pure dedicated server (the player.cpp:2823 decrement loop drives it). Temp probes + bot-give
    REVERTED (`git checkout dlls/ts_gun.cpp`); clean hl.so restored to the rig. Tree = ee642ab.
  - **3e DONE (committed 62d754e) — filled the 1st-person melee/grenade gaps the prediction exposed.**
    Audited the .sc actions: most were already covered once anims predict (fire/reload via SendWeaponAnim,
    empty via the client PlayEmptySound click, pump reload anim, kung-fu CloseCombat). The genuine GAPS:
    knife/katana/sealknife MeleeAttack + GrenadeThrow set only the 3rd-person body anim, never
    SendWeaponAnim → silent, unanimated 1st-person. Fixed: MeleeAttack SendWeaponAnim the slash seq
    (v_knife slash1=3, v_katana/v_sealknife slash1=2 — dumped from the .mdl headers via scratchpad/
    dumpseq.py) + swing/surface/flesh sounds (weapons/knife/*, weapons/katana/*, precached); GrenadeThrow
    SendWeaponAnim(3)=v_m61 throw. FIXED slash (not random) to stay in lockstep with the server's anim
    (random would snap on the reconcile). Verified in-engine: knife slash animates (was static) + sounds,
    no crash. coldcock left server-driven (its viewmodel is the held gun's — no reliable whip seq).
  - **The .sc client weapon-param subsystem (cont.69r NEXT TASK) is COMPLETE:** shared table (1) +
    param cache (2) + CTSGun client-prediction (3a-3d, anims stick, dedicated-verified) + melee/grenade
    predicted anims (3e). Remaining nice-to-haves (not blocking): byte-faithful knife.sc impact spark/
    decal (FUN_10046f50/100492b0), coldcock viewmodel whip, predicted-EFFECT events for the rest.
- **3d** — verify the predicted reload/fire viewmodel anim STICKS (re-run the cont.69s test).
- **3e** — then port the 11 .sc handlers as predicted (now that anims stick), OR — if prediction
  proves too fragile — note that once anims stick via SendWeaponAnim, the .sc events are only needed
  for the muzzle-sprite/shell EFFECTS (EV_TSFire already does those), so 3e may be minimal.

## 2026-06-24 (cont. 69t) — weapon-effect polish batch (impacts/knife-spark/knife-throw) + RE notes

After the .sc prediction subsystem (cont.69s), a user-requested polish batch. All RE-grounded, in-engine
verified, committed:
- **Bullet impact smoke** (8430ea3) — client EV_TSBulletImpact: a SMOKE puff per surface material at
  the bullet hit (RE: client dispatcher FUN_10048f60 — wood/dirt→wood_smoke_add, metal/glass/grate/vent/
  computer→gun_smoke_add+R_SparkShower, concrete/default→gun_smoke, flesh→none). The server side already
  did damage+texture sound+decal+blood; this adds the missing VISUAL. Centre-ray trace; predicted via
  the fire event. Verified: floor/wall impacts puff smoke.
- **knife.sc melee spark** (6876988) — MeleeAttack plays events/knife.sc (m_usMelee, FEV_NOTHOST, impact
  in the origin) on a non-flesh surface hit; client EV_TSMelee R_SparkShower. (RE: FUN_10043c90.)
- **Knife right-click THROW** (dee455c) — RE'd CTSGun::SecondaryAttack@0x817a8 (fire-type KNIFE→KnifeFire
  throw branch) + CreateKnife@0x6d9e4. New dlls/ts_knife.cpp = CTSKnife projectile (per-type w_model,
  MOVETYPE_TOSS + avelocity tumble, damages a victim / thunks a surface, drops a recoverable CTSWorldGun).
  CTSGun::ThrowKnife (spends m_iClip, throw seq v_knife 7 / v_sealknife 5, knife_miss whoosh, auto-switch
  when dry), routed from ItemPostFrame IN_ATTACK2 (gated m_flNextSecondaryAttack). TSThrowKnife + client
  stub. Verified: right-click → throw anim + CTSKnife spawns (w_knife.mdl), no crash.
- **Breakable shatter particles — ALREADY WORKS, no change.** Verified on ts_kissmeyerbasic: punching/
  shooting glass → it breaks + `glassgibs.mdl` loads. Stock HL CBreakable::Die (dlls/func_break.cpp) emits
  TE_BREAKMODEL with the material gib (m_idShard = glassgibs/woodgibs/… by m_Material) + count=0 (engine
  fills the brush volume = covers the surface) + the BREAK_GLASS/WOOD flag. TS uses stock CBreakable (no
  override). If denser coverage is wanted, bump the TE_BREAKMODEL count.

**cont.69t follow-ups (user feedback, committed c16e960):**
- Knife throw COLLISION (RE: CTSKnife::BounceTouch@0x6decc) — STICK into non-breakable non-flesh
  surfaces (embed + point the way it flew); BOUNCE off a breakable it can't break; break-through a
  breakable it CAN break + stick in the wall beyond (forward trace w/ ignore_glass); flesh = stab +
  drop a recoverable knife. **Fixed a server CRASH** (Host_Error "trigger in clipping list") — StickAt
  set pev->solid=SOLID_TRIGGER without relinking; now sets solid+size and relinks (UTIL_SetSize/Origin).
- Pickup = press-E (IN_USE) in a trigger radius, NOT run-over, matching CTSWorldGun::WaitTouch (no aim).
- Dropped-weapon GLOW = WHITE kRenderFxGlowShell (CTSWorldGun + stuck knife) — model shell, not a light.
  (User-corrected: white not orange; E-pickup not run-over.)

**STILL QUEUED (user-requested, RE notes for the next session):**
- **Proximity auto-kick (non-kung-fu)** — "any weapon except kung-fu auto-kicks when next to + aiming at
  a breakable/person." RE is HARD: the kick is `CloseCombatAttack__11CBasePlayer` (@0x95f88) called via an
  INDIRECT function pointer (PTR_CloseCombatAttack…@0x176858) — static caller not found. The per-frame
  trigger (proximity + aim check) isn't in CTSGun::PrimaryAttack (0x81a20, pure fire-type dispatch) and
  CTSGun has no ItemPostFrame symbol (uses base). Kick helpers RE'd: VKickback@0x80414,
  GetCloseCombatVectors@0x86e2c, DropKickback@0x86e9c, GaitCloseCombat. NEXT: trace the indirect ptr / find
  the CBasePlayer think that calls it; don't guess the range/damage (RE-ground it).
- **Coldcock viewmodel whip** — ColdCock has the hit sound + 3rd-person anim but no 1st-person SendWeaponAnim;
  most gun v_models lack a whip seq. RE coldcock.sc (LAB_10043800) for the seq it uses (may be a generic).
- **Predicted-effect events for empty/pump/closecombat** — low value (anims already predict; empty already
  clicks via PlayEmptySound; pump scock1 + closecombat sounds are server-side EMIT_SOUND).
- **Katana right-click** = KatanaThrow (separate from the knife throw) — not yet done.

## 2026-06-23 (cont. 69r) — committed the batch; chose the FULL .sc client-prediction subsystem as next

Committed everything (git): **1ca0ca4** (akimbo refactor + kung-fu + all playtest fixes — cont.69p/q),
**c675ef0** (reference/EVENTS.md = complete RE map of the 11 .sc weapon events, both sides), **7a66e5c**
(EVENTS.md = client weapon-param subsystem foundation + build order).

RE'd the `.sc` event system end-to-end: the files are empty 4-byte hooks; server `PLAYBACK_EVENT` <-> client
`pfnHookEvent` handler. Map in EVENTS.md — server handles (CTSGun::Precache@0x80660: fire+0x964 … grenade
+0x978), client hook->handler (FUN_10003240: fire->FUN_1003de40, reload->LAB_10042690, closecombat->FUN_10043200,
… 11 total), playback sites. Only `events/fire.sc` is wired today (EV_TSFire); the rest are server-driven.

**User decision:** do the FULL client weapon-param subsystem (byte-faithful), not the simplified per-event
path. Foundation RE'd: client `get_weapon_info` = FUN_100071c0 -> DAT_100aa050 (the SAME 988-byte
s_weapon_info[38] the server builds) + the DAT_10189xxx active-param cache (filled on deploy, read by the
handlers). Build order (EVENTS.md): (1) client weapon table, (2) deploy param-cache, (3) the 11 handlers.
NOT started (deferred to a focused session — it touches the working weapon code). Added scripts/ghidra/
FindStr.java (find a .data string + its xrefs). See HANDOFF "NEXT TASK".

## 2026-06-23 (cont. 69p) — akimbo OWNERSHIP refactor IMPLEMENTED + 3 playtest fixes

Implemented the user-approved akimbo-ownership refactor (the cont.69o hand-off). CTSGun's arsenal
migrated from the compact `m_arsenalType[]` list to the original's **items[]-by-tsweapon_t model**
(`struct TSItem m_items[TSW_COUNT]` = {owned, clip, leftClip, rightClip, mode, attach, barrelMask}),
all RE-grounded (TakeWeapon@0x80a04, CTSGun::DropWeapon@0x8094c, CBasePlayer::DropWeapon@0x84900,
SwitchItems=FUN_00081564, GetBetterWeapon@0x80f40, IsWeaponAkimbo/IsOnlyAkimbo = fireType 4/5,
g_iWeaponPriority extracted from compile_weapons_info). dlls/ts_gun.cpp + ts_weapons.h + client.cpp.
- **Combine keeps BOTH owned** (TakeWeapon): own single + pick up another → fill items[akimbo]
  (leftClip = items[single].clip, rightClip = weapon_info[single].clip), single NOT cleared. Scroll
  skips the single (akimbo owned), slot1-5 still selects it. **Verified in-engine** (`tsbuy 2` ×2 →
  akimbo berettas in hand; slot2 → single beretta while still owning the akimbo).
- **Drop branches** (CBasePlayer::DropWeapon, now all in TSDropWeapon): combinable akimbo → throw 2
  base singles (clip/2 each) + remove both items[akimbo]/[single]; **single dropped while owning its
  akimbo → throw 1, keep the single (clip = akimbo.rightClip), remove the akimbo** (the cont.69o
  REMAINING item) — **verified in-engine**; plain → throw 1 + remove. GetBetterWeapon auto-selects by
  g_iWeaponPriority. world-gun spawning moved server-side into TSDropWeapon.
- **useitems** (SwitchItems) cycles the active akimbo's barrel mask 0→2(L)→4(R)→6(both)→0; AkimboFire
  decrements per-barrel clips honouring the mask; reload redistributes across barrels.
- SelectWeaponType (holster→deploy), lastinv, WStatus, death-scatter, pickup all keyed by type now.

**3 playtest fixes (all user-reported, RE-grounded, rebuilt):**
1. **Spawn→spectate bounce was NOT a code bug** — the test launch missed `+deathmatch 1 +maxplayers`,
   so the listenserver ran single-player rules and stock `respawn()` (client.cpp) does
   `SERVER_COMMAND("reload")` → endless map-reload loop (looked like a crash; gdb caught no SIGSEGV).
   Confirmed: committed build does the same; `+deathmatch 1` → spawns clean. **CLAUDE.md §3 launch
   command updated** to require the flags.
2. **Kung-fu no longer drawn red** in the weapon-select bar (cl_dll/ts_weaponbar.cpp): the empty-clip
   red tint now only applies to weapons with maxClip > 0 (clipless melee has no ammo to be out of).
3. **Dropped weapons no longer block the player while falling** — RE confirmed (CTSWorldGun::Spawn
   @0xa0954) the original gives the FALLING gun a ZERO-size bbox (SetSize DAT_00106a08=0.0), so the
   SOLID_BBOX point drops+lands but the player walks through (it damages via the Fall trace, not
   collision); real trigger size only on landing. ts_worldgun.cpp falling SetSize → g_vecZero.

Built hl.so + client.so (glibc 2.14), staged via setup-xash-mod, in-engine verified (no crashes,
0 reconnects with +deathmatch 1). NOT yet committed. Remaining: exhaustive 38-weapon retest.

**Follow-up fixes (same batch, user playtest):**
4. **Loadout now PERSISTS across spawns** — removed the `ResetArmament()` we had added at the end of
   `CBasePlayer::BuyArmament()` (ts_buy.cpp). The original BuyArmament@0x83f10 does NOT reset, so the
   chosen loadout re-applies on every spawn (respawn + round restart). rst_cfg still clears on rebuy.
   Fixes "round restart gives zero weapons".
5. **No more phantom beretta** — `CTSGun` default `m_weaponType` changed TSW_BERETTA → **TSW_MELEE**, so
   a bare `weapon_tsgun` (GiveNamedItem) is just kung-fu; the loadout supplies the real guns.
7. **FIRING fixed (fire-once-then-stuck, reload never refilled)** — the build defines CLIENT_WEAPONS so
   `UTIL_WeaponTimeBase()` == 0 (a 0-based predicted clock). The original CTSGun is client-predicted, but
   our reconstruction drives it SERVER-authoritatively, so every CTSGun timer compared a positive
   `m_flNextPrimaryAttack`/`m_flTimeWeaponIdle`/reload time against 0 → never elapsed → one shot then
   permanently stuck, reload anim played but clip/reserve never updated. Converted all CTSGun timers to
   `gpGlobals->time` (and `GetNextAttackDelay(x)` → `gpGlobals->time + x`). In-engine verified: M4 full-auto
   sustained fire (muzzle flash + brass), reload refills.
9. **MUZZLE-FLASH "flares" added (were not drawn)** — RE of client.dll: the muzzle flash is the TS
   sprite drawn at the gun muzzle ATTACHMENT by `FUN_100474b0` (loads `sprites/ts_muzzleflash1h.spr` =
   one-handed, `ts_muzzleflash6.spr` = two-handed; a 2nd flare when `weapon_info+0x350` is set). Our
   EV_TSFire only did the engine EF_MUZZLEFLASH (and akimbo fired no event at all). Now `EV_TSMuzzleFlare`
   draws the TS sprite via `R_TempSprite` at the viewmodel/player `attachment[side]` (per-grip sprite),
   and AkimboFire fires `events/fire.sc` with `iparam2 = barrel side` so each akimbo barrel flares at
   attachment 0/1 (gold colts = akimbo-only, both barrels). Sprites precached in PrecacheAllTSWeapons.
   In-engine verified (beretta flare at the muzzle). NOTE: a faithful approximation — the original's exact
   multi-segment temp-sprite build (FUN_100474b0, scale from weapon_info+0x1c/+0x354/+0x358) is not fully
   reproduced; flare scale/offset may need tuning. SHOTGUN incremental reload (ShotgunReload@0x84aac, per-
   shell state machine) still TODO.
8. **BATCHED LOADOUT fixed (selected arsenal not given)** — RE of the buy-menu apply (client.dll
   FUN_10039400) shows it builds ONE space-separated command `"tkw "` + `"%i_%i "` per weapon
   (`tkw 2_1 5_1 9_1`) + one `tka …`, sent via a single ClientCmd. Our server `tkw`/`tki`/`tka` handlers
   only parsed `CMD_ARGV(1)` → dropped all but the first weapon. Fixed to loop `CMD_ARGV(1..ARGC-1)`.
   With the persist fix (#4) the full loadout now applies on every spawn + round restart. In-engine
   verified (batched `tkw 2_1 5_1 9_1 13_1` → all given). BUY.md note corrected (was wrong: "single token").

6. **Can't-fire / no-viewmodel fixed (regression from #5)** — the engine's auto-deploy (GetNextBestWeapon)
   skips kung-fu (melee, no clip), so with a kung-fu default the tsgun never became `m_pActiveItem` →
   no viewmodel + ItemPostFrame (firing) never ran. Added **`TSDeployArsenal(player)`** (ts_gun.cpp,
   called at the end of CTSGameRules::PlayerSpawn) which forces the tsgun active + Deploy()s; also made
   `SelectWeaponType` claim `m_pActiveItem` when none is set. **In-engine verified**: M4 viewmodel
   deploys, loadout pistol deploys on spawn, 0 reconnects. (Before this, BERETTA default masked the bug
   because the engine auto-deployed a gun.)

## 2026-06-23 (cont. 69q) — kung-fu close-combat, shotgun reload, muzzle flares, fire/loadout/deploy fixes

Big playtest-driven batch (all RE-grounded; in-engine where verifiable):
- **CanDeploy fix** — TS weapons keep ammo in our own m_ammoReserve, not stock m_rgAmmo, so the stock
  CBasePlayerWeapon::CanDeploy refused to deploy any empty-clip TS weapon — notably KUNG-FU (always 0
  clip), which then kept the PREVIOUS weapon's viewmodel ("kung-fu takes prev model" / "round restart
  strips kung-fu"). `CTSGun::CanDeploy()` now returns true. Verified: weapon_0 → fists.
- **KUNG-FU close-combat** (`CCombatAttack__6CTSGun`@0x86078): kung-fu = bare hands (original has gun+0xcc==0
  → CCombatAttack). PrimaryAttack/Secondary for TSW_MELEE now route to `CloseCombatAttack()` — a contextual
  dispatch from pev->button (IN_ATTACK/IN_ATTACK2 + IN_DUCK) + FL_ONGROUND/FL_DUCKING + velocity → the
  v_melee.mdl anims (0x0d-0x19) + damage 35/45/55: right-click+duck=handstand kick(0x14), right-click=back-
  kick(0x11), fire+air+duck=dropkick(0x10), fire+air=flying punch(0x12), fire+duck=uppercut(0x16),
  fire=ground punch combo(0x0d/0e/0f). Sounds (RE: player/*): flesh→kungfuhit.wav, surface→closecombat.wav
  swing + pl_step* footstep, miss→closecombat.wav swing. (SLICE: exact knockback/reach consts approximated.)
- **SHOTGUN incremental reload** (ShotgunReload@0x84aac): pump shotguns (fireType TSFT_PUMP: m3/spas12/
  mossberg) now reload ONE shell at a time (per-shell delay, scock1.wav), interruptible by firing.
- **MUZZLE FLARES** (RE: client.dll FUN_100474b0 + ts_muzzleflash1h/6 sprites): EV_TSMuzzleFlare draws the
  TS flash sprite at the gun muzzle attachment (akimbo per-barrel via iparam2). Flare life shortened (was
  "slow enough to walk around" — dropped FTENT_SPRCYCLE, life 0.01).
- **FIRING** (server-time gate, was fire-once-stuck) + **batched-loadout** (tkw loops all argv) + **loadout
  persists** (no ResetArmament) + **deploy** (TSDeployArsenal) — see cont.69p notes above; all RE-grounded.
- **ROUND-RESTART WEAPON LOSS fixed (recurring bug)** — `CBasePlayer::Spawn` does NOT call
  RemoveAllItems, so TSRestartRound respawning a STILL-ALIVE player left the old weapon_tsgun in place
  and GiveNamedItem made a SECOND one; the two tsguns fought over active/WStatus → the arsenal
  "disappeared" inconsistently (all-but-one / all). Fixed: CTSGameRules::PlayerSpawn now
  `RemoveAllItems(false)` before GiveNamedItem so the round-restart re-equip is clean (one tsgun).
- **KUNG-FU anims FIXED** — our v_melee.mdl has only 11 seqs (idle/draw/punch_left/punch_right/block_low/
  kick/sidekick/uppercut/spinkick/backkick/punch_right2); I'd used the original's 0x0d-0x19 which don't
  exist → no animation. Remapped CloseCombatAttack to our seqs (punch combo 2/3/10, uppercut 7, kick 5,
  sidekick 6, spinkick 8, backkick 9). Reload "hooah" block → block_low (4).
- **KUNG-FU reload block IMPLEMENTED** — CloseCombatBlock: R plays player/block.wav ("hooah") + block_low
  stance + gates attacks; a stamina bar (~3 blocks, regens) → player/breathe2.wav (the Valve breathing)
  when spammed empty. Sounds in ts/sound/player/.
- **`thirdperson` is now a TOGGLE** (cl_dll/in_camera.cpp CAM_ToThirdPerson): flips third↔first; `firstperson`
  stays one-way.
- **Kung-fu 3rd-person held-weapon removed** — CTSGun::Deploy fell back to the VIEW model when a weapon had
  no p_model, so kung-fu (no p_model) put v_melee on the player's hands/back (a phantom held gun). Now an
  empty weaponmodel ("") → bare hands. NOTE: the player BODY kung-fu animation (punch/kick on the 3rd-person
  model) needs kung-fu sequences in the player model + the CTSAnimation close-combat gait (GaitCloseCombat/
  HandleCloseCombat@0x5d8d0); the current player model (SpeziClaus) only has onehanded/twohanded/akimbo aim
  sets — no fist set — so the body can't punch visually with it. Deferred (model-dependent).
- **KUNG-FU dispatch reworked + 3rd-person BODY anims** (user rules): FIRE = PUNCHES only (never kicks,
  even with jump) — duck→uppercut, hold/rapid→punch combo (punchleft/right/air sequence), lone tap→single
  regular punch; +ATTACK2 = KICKS only — jump+duck→spinkick, duck→sidekick, back→backkick, else→kick. The
  player models DO carry the kung-fu sequences by NAME (gordon: punchleft/punchright/punchair/uppercut/
  sidekick/backkick/circkick/circspin/kungfu_block1, …) — added `CTSAnimation::PlayKungFu(player, seq)` and
  each attack/block now plays its body sequence (held via the aim-timer) so 3rd-person shows the punch/kick,
  not an aim pose. (Earlier model-parse used the wrong regex and missed these; the named seqs exist.)
- **STILL TODO (RE'd, not implemented):** kung-fu reload = `CCombatBlock`@0x85ebc — a "hooah" taunt
  (event gun+0x96c, anim 0x26) that blocks attacks (sets gun+0x9c/0xa0) + consumes stamina (player melee
  bar, UpdateMeleeBar); spamming → low-stamina breathing sound. The closecombat client event handler
  (FUN_10043200) for the swing-anim prediction. The flare multi-segment build (FUN_100474b0 exact).

## 2026-06-23 (cont. 69o) — full akimbo-ownership model RE'd (refactor handed to next agent)

User asked to ground the akimbo logic in RE and (after I flagged the scope) chose to do the full refactor;
we decided to hand the IMPLEMENTATION to the next agent (this session was already very long). The complete
model is RE'd + documented in reference/WEAPON_SELECT.md ("Akimbo ownership model" + "TakeWeapon combine"):
TakeWeapon@0x80a04 (combine: akimbo item carries two barrel clips +0x130/+0x134; KEEPS both items[single]
and items[akimbo] owned), DropWeapon@0x84900 (drop akimbo → 2× base single), SwitchItems=FUN_00081564
(useitems toggles the active-barrel mask +0x14 bits 2/4), GetBetterWeapon@0x80f40 (g_iWeaponPriority).
NEXT TASK = migrate CTSGun arsenal from the compact m_arsenalType[] list to items[]-by-tsweapon_t so a
single can be wielded/dropped independently of its akimbo pair. See HANDOFF.md "⭐ NEXT TASK".

## 2026-06-23 (cont. 69n) — death=spectate, spectator-chrome suppression, weapon-select rebuild

Playtest fixes (all RE-grounded + in-engine/user-confirmed):
- **Death re-enters spectate** (not timer-respawn): PlayerDeathThink, once the death anim finishes,
  routes to TSEnterSpectate (free-look + buy menu); respawn on FIRE after the gate. Verified via `kill`.
- **Spectator chrome suppressed in the TS phase**: the top-left green-grid overlay = the stock
  CHudSpectator overview/inset (DrawOverview, gated off in HUD_DrawNormalTriangles while
  m_Spectate.IsActive()); "Press DUCK for Spectator Menu" (#Spec_Duck, SpectatorPanel::ShowMenu)
  suppressed. The dark bottom bar = the stock SpectatorPanel bottom border geometry (height
  YRES_HD(PANEL_HEIGHT=64), mode at YRES_HD(6)) drawn via pfnFillRGBABlend (pfnFillRGBA is additive →
  black was invisible); user-confirmed match.
- **Config-apply flicker fixed**: the buy menu opened every spectate tick (server resends each
  countdown second) → reopened after apply. Now opens once on entering spectate. Kung-fu (TSW_MELEE)
  given on every spawn (BuyArmament). Status panel hidden while dead/spectating.
- **Weapon select REBUILT** (the stock yellow CHudAmmo menu retired; TS bar is the select UI). RE'd
  from FUN_1004f500/FUN_1004f220/FUN_1004d2cf/FUN_1004e030 (reference/WEAPON_SELECT.md): slot1-5 +
  invnext/invprev open the bar + move a cursor; FIRE commits `weapon_<N>` (holster→deploy, plays
  weapons/switch.wav); closes on commit/cancel (NO timer). Bar ALWAYS draws but very transparent;
  selecting brightens it, cursor most opaque (3-level additive opacity, no box); cursor shows
  curclip/maxclip. Mouse-scroll SKIPS kung-fu (type 0) + a single whose akimbo is owned (still
  slot-reachable). +attack consumed on commit (m_iKeyBits, like CHudAmmo::Think).
- **Kung-fu = fists** (v_melee.mdl, no clip; was the MK23 placeholder). **Dropping an akimbo throws
  TWO base singles** (clip split, ±4; CBasePlayer::DropWeapon @0x84900, TSAkimboBase = reverse iAkimboType).
- OPEN (need the akimbo-as-2×-single ownership model, our model is akimbo-as-one-slot): "drop the
  single (while holding akimbo) leaves the other"; the per-side single ownership.
- Commits 269420b/2bac9c7/96d1e93/58991ef/2f769d9/504d62f/43880b9/a679c18.

## 2026-06-23 (cont. 69m) — buy-menu config flow, TSMessage notifications, spectate HUD polish

User playtest feedback round (all RE-grounded + screenshot/user-confirmed):
- **First-start random + config toggle** (FUN_10036670 = menu open): first open per map randomizes the
  loadout → "Use New Config"; later opens reload the last-applied config → "Use Last Config". Open() guards
  on already-open (the server resends the spectate msg each countdown tick). Apply() saves the last config.
- **TSMessage SVC notification** (cl_dll/ts_message.{cpp,h} = CHudTSMessage): RE'd the handler chain
  FUN_100499e0 → FUN_1004c0a0 (parse: byte R,G,B,flags,holdtime + string) → FUN_1004b520 (15-slot centred
  queue). Our server already sends TSMessage (ts_utils.cpp); this adds the client handler (fixes the "No pfn
  TSMessage" error). Centred just below screen centre (flags 0); 0,0,0 → light text; **fades out** as it
  expires (additive font dimmed over the last second). The buy "Using new/last config." notice + the
  spectate "Press fire to play" prompt go through it ("press fire" is now a one-shot fade on the
  countdown→READY transition, was a permanent orange overlay).
- **Spectate bottom bar**: the original's dark bar IS the stock SpectatorPanel bottom border — geometry
  matched (height YRES_HD(PANEL_HEIGHT=64), bottom-anchored, "Free-Look" mode at YRES_HD(6), help near the
  bottom). Drawn with **pfnFillRGBABlend** (pfnFillRGBA is additive → black was invisible). User-confirmed match.
- **Status panel** (CHudTSStatus) hidden while dead/spectating (gHUD.m_Spectate.IsActive()).
- Commits 47d91fd (config/notify/fixes) / 2f769d9 (bar geometry).
- **AUTO-AMMO max field** captured from the LIVE hl.exe (PID via /proc/<pid>/mem, client.dll base
  0x0c070000, caliber table @0x0c10ffc0) — the static-0 max field [3]; see cont.69l + reference/MENUS.md.

## 2026-06-23 (cont. 69l) — REAL cascade buy menu REBUILT as a HUD element + verified vs user refs

Rebuilt the real cascade buy menu (the `0x10037xxx` cluster RE'd in 69k) as **`CHudTSBuyMenu : CHudBase`**
(`cl_dll/ts_buymenu.{cpp,h}`), replacing + **deleting** the legacy vgui `vgui_TSBuyMenu.*`. Wired into
hud.{h,cpp}/CMakeLists; input via HUD_Key_Event (keys) + IN_MouseEvent (mouse); opened by the `buy` cmd +
the spectate-wait Spectator msg; cursor freed via UpdateCursorState.

**Completed the RE first (reference/MENUS.md "★★ THE REAL BUY MENU"):** the obj field map (obj=HUDobj+0x7c0),
the full input state machine **FUN_10038840** (levels 0-8: config/category/weapons/detail/accessory/remove/
save/load/changeteam), the **mouse** path **FUN_10038360** (left=entry/miss=key1/right=key10) + hit-test
**FUN_10038410** (col=(mx-8)/0x80, row=(my-ScreenH/2)/0x14), accounting (budget 30000, slot cap 81, costs
FUN_10012ad0/FUN_10038dc0), apply **FUN_10039400** (rst_cfg + batched tkw/tki/tka), and the full weapon
cost table (price/slot/fullcnt extracted from the 36 init funcs; support/akimbo from the attach RE; hudCol/
sprite from the weapon-bar). Builders FUN_10037540/100377d0/10037960/10037ba0 + header/preview FUN_10036af0.

**Drawn faithfully + screenshot-verified** vs the user's 5 reference images: real roster (Beretta 92F #5,
Contender G2 #10 in Handguns; "Special Purpose"; "Go Back"), green "7 Random", blue Cancel, per-entry
availability colours, the **1-5 loadout-preview columns** (bought items by HUD column in add-order, count-
left-of-icon — Seal Knife 10 / Combat Knife 5 — Kevlar interleaved, `(Sil ,Las ,Fla ,Sco )` line),
**`Free Slots = 81 - used`** (remaining, not used), Total = weapon+attach (×fullcnt for full-load). All
match (Total 6300 = 500+1000+2900+1900; Free Slots 46 = 81-35). **Mouse confirmed working by the user.**
Save/load configs, Remove-Item, Change-Team, Random, Reset all implemented per the state machine.

**Auto-ammo DONE (runtime-captured).** The menu auto-fills each weapon's caliber to max, adds the cost to
Total, shows rounds left of the preview icon (Glock 210 / M16 120), emits `tka <cal>_<units>`. The per-caliber
**max** field reads 0 statically (set only at load), so the caliber table `DAT_1009ffc0` + per-weapon caliber
(`weapon_info+0x08`) were read from the LIVE hl.exe via `/proc/<pid>/mem` (client.dll base 0x0c070000, table
@0x0c10ffc0) — full table in reference/MENUS.md "AUTO-AMMO". Verified Glock → 350cr → Total 3400.

**Spectate HUD fixed.** RE-verified TS has NO stock spectator chrome (no top "Map:" bar) — inherited HL/TFC
`SpectatorPanel` was bleeding through. `SpectatorPanel::Update()` suppresses the top+bottom borders + Map/
timer/scores during TS spectate; `CHudSpectate::Draw()` draws the dark bottom bar + "Free-Look" + control help
in the HUD layer (was hidden under the vgui overlay). Screenshot-verified vs the original. Commits 258a447
(menu) / f17a453 (ammo) / 2bac9c7 (spectate).

## 2026-06-23 (cont. 69k) — DISCOVERY: we RE'd the WRONG (legacy) buy menu; full RE of the REAL one

User supplied reference screenshots of the real cascade buy menu (Config|Categories|Weapons|Detail columns
all visible, "Special Purpose", "Go Back", attachment costs, Beretta 92F + Contender G2 in Handguns). NONE of
that matched my menu RE. Root cause: the TS client.dll has TWO buy-menu code paths — the **legacy/dead** one
I RE'd + built (`0x10010xxx`, vgui::Panel, literal strings, `DAT_1007a950` table) and the **REAL** one the
game actually draws (`0x10037xxx`). All on-disk client.dll copies (incl. the Steam install) are byte-identical
(688128b) — same binary, I just RE'd the wrong builder. **User chose: RE + rebuild the real cascade menu.**

**Full RE of the real menu (cont.69k, all in reference/MENUS.md "★ THE REAL BUY MENU"):**
- It's **HUD-drawn, NOT vgui**: entries via `FUN_1004ff10` → `FUN_10021ce0`(=`TSHud_DrawString`) + the TS HUD
  font `DAT_100d5908` + `titles.txt` token localization (`FUN_100349a0`). So it reuses our existing HUD draw.
- **Cascade**: orchestrator `FUN_10036750` (switch on `obj+4` = level: 0 config/1 category/2 weapons/3 detail/
  4-7 accessory&c/8 off). Columns side-by-side at X=8/0x88/0x108/0x188, rowH 0x14, width 0x80. `FUN_10036af0`
  draws the top header (Total + Free Slots). Builders: config `FUN_10037540`, category `FUN_100377d0`,
  weapons `FUN_10037960`, detail `FUN_10037ba0`.
- **Colours**: available = half hud_r/g/b (dim); over-budget = red (195,45,45); Random/affordable-Buy = green
  (45,195,45); Cancel/GoBack = blue (100,100,205). "7 Random" is green BY DESIGN (not hover).
- **Real weapon roster** (the menu's own table `obj+0x2d6c`, indexed row*5+cat, filled by `FUN_10036200` — NOT
  DAT_1007a950): Handguns [1,9,12,14,2,21,22,28,31,36] (Beretta 92F #5, Contender G2 #10 — matches the image!),
  SMGs [3,6,7,17,19,23], Rifles [5,13,15,27,18], Shotguns [4,11,20,26,33], Special Purpose [24,25,32,34,35].
- **Attachment costs** (detail, "%i <name> %i" = key+cost): Silencer 900, Lasersight 800, Flashlight 500,
  Scope 1800. Availability via `FUN_10038d70` (same 0xE0/0xE4 weapon masks).

**NEXT: rebuild as a CHudBase HUD element** (header + cascade columns via TSHud_DrawString + TS font + the
roster/tokens/colours above), replacing the vgui CTSBuyMenuPanel. Still to RE: the input/level-advance handler,
the accessory column (states 4-7), the `FUN_10038180` weapon-preview geometry. The legacy vgui menu + its
cont.69g-j work is superseded for structure (the styling/colour findings carry over).

## 2026-06-22 (cont. 69i) — per-weapon attachment submenu RE'd + built + verified

User: clicking a weapon should open a per-weapon submenu to buy attachments; RE it. Done.

**RE (client.dll):** the 4th panel `+0x2d90` is built by **`FUN_10010bd0`** (state +0xc=4). It reads the
weapon (+0x28) + selected-attachment mask (+0x24) and builds: "1 Buy" (action 8), each available
attachment "Silencer/Lasersight/Flashlight/Scope" (action 10 = toggle bit, shown if `support & ~exclude &
~selected`), then "6 Buy Akimbo" (action 8) if akimbo OR "6 Full Load" (action 9), then "0 Cancel".
**The attachment availability is WEAPON DATA** (and the same data drives the HUD attached-icon row, per the
user): weapon-struct fields `0xE0` support mask / `0xE4` exclude mask (bit0 Sil,1 Las,2 Fla,3 Sco) / `0xC8`
akimbo type / `0xDC` full-load flag. Extracted the effective mask + akimbo + full for all 36 weapons from
their init funcs (e.g. MP5SD/AUG/Barrett/Ruger *exclude* their integrated silencer/scope). Full table in
reference/MENUS.md "Per-weapon attachment submenu — RESOLVED".

**Built (cl_dll/vgui_TSBuyMenu, `PAGE_WEAPON_DETAIL`):** clicking a weapon now opens the attachment submenu
(built from a new `s_weaponAttach[]` table); attachments toggle client-side (selected ones disappear);
Buy/Buy Akimbo/Full Load queue the weapon (`tkw <id>_1`, akimbo → variant id); Cancel → back to the list.
**In-engine verified** by mouse+key nav: detail renders Buy+attachments+Cancel, and MP5SD correctly shows
Lasersight/Flashlight/Scope (silencer excluded — RE-accurate). Both client arches build clean.
**DEFERRED (next slice):** the attachment-to-server wire encoding (the tangled `FUN_10012050` apply
string-builder) + server-side attachment application + the HUD attached-icon row — attachments are selected
client-side but not yet networked/applied.

## 2026-06-22 (cont. 69h) — buy-menu interaction fixes (close/reopen/cursor/mouse-click)

User-reported bugs on the live menu, all fixed + in-engine verified:
- **Close:** config items 1/7/0 (Use New Config / Buy Random / Cancel — server TSMenuSelect cases 1/7/10)
  now hide the CLIENT panel via a new `CTSMenuHandler_SelectClose` (sends the menuselect cmd + `ShowTSBuyMenu(false)`).
  Before, they used `CMenuHandler_StringCommand(..., true)` → `HideTopMenu()`, which only closes the viewport's
  command menu, not our panel, so the menu persisted.
- **Reopen:** the client `buy` command (bound `b`) was forwarding to the server's now-no-op `TSSendBuyMenu`;
  it now reopens the client vgui panel directly (`gViewPort->ShowTSBuyMenu(true)`), gated on
  `gHUD.m_Spectate.IsActive()` (spectate-wait only). `cl_dll/ts_commands.cpp`.
- **Cursor:** `UpdateCursorState()` didn't include the buy menu, so the player stayed locked to aiming and
  couldn't click. Added `(m_pTSBuyMenu && m_pTSBuyMenu->IsUp())` to the "need cursor" condition.
- **Mouse-click on entries (the hard one):** clicks reached NOTHING in our panel (proven with a temp
  InputSignal that flips the box red on mousePressed — it never fired). TWO fixes: (1) the panel was a small
  child that lost vgui mouse-focus → rebuilt as a **full-screen panel** (0,0,ScreenWidth,ScreenHeight) with
  the box (a child `m_pBg`, resized per page) + entries at the menu origin inside it — the class/team-menu
  pattern; (2) it was created 2nd in `Initialize()`, BEHIND the later full-screen SpectatorPanel/ScoreBoard
  in z-order, which captured the mouse → moved `CreateTSBuyMenu()` to **LAST** so it's the topmost sibling.
  After the reorder the diagnostic box turned red (clicks reach the panel); removed the diagnostic.
  **Screenshot-verified:** clicking "2 Buy" → category page; clicking a category → its weapon list; the
  bound number keys still work too. Both client arches build clean.

## 2026-06-22 (cont. 69g) — per-category weapon lists RE'd + built + in-engine verified

Closed the buy menu's remaining static-RE slice: the per-category weapon lists (Handguns/SMGs/Rifles/
Shotguns/Other). **All RE in reference/MENUS.md "Per-category weapon lists — RESOLVED".**

**RE (client.dll, via the persistent Ghidra client project):**
- The 3 "unmapped" action thunks resolved: `FUN_10010890`/`FUN_10010bb0` are just panel-hide stubs; the
  real builder is **ONE** function **`FUN_100108b0`**, parameterised by category at `clientobj+0x20`,
  rebuilding the `+0x2d8c` weapon-list panel. It iterates a `[cat][9]` weapon-id table **`DAT_1007a950`**,
  drops -1/restricted/(M61=id24 in non-team mode, `DAT_100cf97c>1` gate), labels `"%i %s"` green/red by
  affordability, ends with `0 Cancel` (100,100,205). Dumped the exact table from the PE:
  Handguns[1,9,12,14,8,21,22,28,31] SMGs[3,6,7,17,19,23] Rifles[5,13,15,27,18] Shotguns[4,11,20,26,33]
  Other[24,32,34,25,35].
- **Display names** = weapon struct `+0x44` (client table `DAT_100aa050`, stride `0xf7*4=0x3DC`,
  `FUN_100071c0`). Decompiled all 36 per-weapon init funcs (dispatcher `FUN_10007de0`) + resolved the
  `+0x44` source string for each → full id→name map (e.g. 3=Mini-Uzi, 19=MP7-PDW, 32=M60E3, 5=M4A1,
  27=M16A4, 15=STEYR-AUG, 25=Combat Knife, 35=Seal Knife — several differ from our server pszName).
- Confirmed via objdump that the entry label is `sprintf("%i %s", onscreenIdx+1, name)` and the entry-add
  is `FUN_100668f0(panel, idx, label, r,g,b, 0x32, scale, action=8)`; the availability green/Cancel-blue
  colours match the prior FUN_10011160 RE.

**Built + in-engine verified (cl_dll/vgui_TSBuyMenu.{h,cpp}):** a dynamic `PAGE_WEAPONS` rebuilt by
`ShowCategory(cat)` from the RE'd table (+ the M61 team-gate via `gViewPort->GetNumberOfTeams()>1`); the 5
buy-category buttons navigate to it; each weapon click emits the real **`tkw <id>_1`** (queues into the
server armament; commits on "Use New Config"→spawn `BuyArmament`); `0 Cancel` → back to category.
Reusable 10-slot page with one persistent per-slot action reading the live `m_weaponId[]` (categories swap
in place). **Screenshot-verified on tm_avana**: Handguns shows all 9 (Glock-18…Raging Bull) + blue Cancel,
SMGs shows the 6, single green numbers (the SDK CommandButton draws the bound-key column, so labels are the
bare name — matching the config/category pages). Fixed: panel `tall` 8→10 rows + per-page `setSize` so the
bg box hugs the active page (Handguns is 10 rows; was clipping rows 9-10). Both client arches build clean.

**STILL UNFINISHED:** (1) the per-weapon **detail submenu** (`6 Full Load`/`6 Buy Akimbo`, panel `+0x2d90`,
action 8's real target) + the faithful client-side **loadout queue + apply sequencer** (FUN_10012050 emits
rst_cfg+the tkw/tki/tka list at once — we bridge per-click); (2) the client **free-slots** red/unavailable
gate (`+0x430` vs 81); (3) the config-panel EXACT colours/positions (the runtime hbreak capture — still
needs the user's clean Proton relaunch; procedure in MENUS.md "Runtime capture").

## 2026-06-22 (cont. 69f) — buy-menu vgui reproduction (4 of 5 pages) + runtime-debug findings

Built the client-side buy menu as a faithful vgui reproduction (cl_dll/vgui_TSBuyMenu.{cpp,h} =
`CTSBuyMenuPanel`, owned by the viewport, shown in spectate-wait), and dug into the live original to crack
the config-panel exact styling that static RE couldn't reach. **All RE in reference/MENUS.md.**

**Built + committed (compile + render in-engine):**
- 3 menu PAGES as toggled vgui button sets with client-side page nav (`CTSMenuHandler_Page`): **Config**
  (8 items → server `menuselect`), **Buy-category** (RE'd FUN_10010440: Handguns/SMGs/Rifles/Shotguns/Other/
  Player Accessory/Cancel), **Player Accessory** (RE'd @0x10010dd0: Kevlar/Silencer/Lasersight/Flashlight/
  Scope; Kevlar→`cl_buykevlar`, ties to the kevlar gameplay). `CTSMenuButton` = CommandButton w/ the TFC
  orange suppressed (RE: plain vgui::Button look). Retired the server gmsgShowMenu stand-in.
- **Number-key routing** (HUD_Key_Event → `CTSBuyMenuPanel::HandleKey`: fires the visible page's button whose
  `getBoundKey()` matches; both click AND key now work, matching the original).

**Runtime-debug findings (the original TS runs live under Proton — gold for the remaining unknowns):**
- gdb attaches to the live game (`hl.exe -game ts`, find/x memory reads work; client.dll base **0x0c070000**,
  runtime = static − 0x3F90000). **SOFTWARE breakpoints (int3) silently DON'T fire** in this Proton/wow64
  process; **HARDWARE breakpoints (`hbreak`) DO** (proven: hbreak on HUD_Redraw @0x0c08aa90 fired). Verified
  FUN_100668f0 @0x0c0d68f0 byte-for-byte.
- **KEY: the vgui menu is built ONCE at first client init and fully CACHED for the process lifetime** —
  surviving map loads, reconnects, AND vid_restart/res-change (res-change only `setPos`-repositions the
  cached buttons; it does NOT recreate them, so the create-breakpoint FUN_100668f0 never re-fires). So the
  ONLY way to capture the build args (real per-button text+RGB+pos) is: arm the hbreak BEFORE the very first
  build → fully quit+relaunch the game, connect, then open the buy menu the first time with the hbreak
  already armed. Procedure is in MENUS.md ("Runtime capture — the exact procedure"). NOT YET EXECUTED (the
  user hit SDL-init errors relaunching — leftover wineserver/hl state from the attach/detach churn).
- **Weapon-list builders identified** (MENUS.md action-thunk map): `FUN_10010890` / `FUN_100108b0` /
  `FUN_10010bb0` (the unmapped action-`0xb` thunk targets) — decompile these for the per-category weapon lists.

**STILL UNFINISHED:** (1) per-category weapon LISTS (build the 5 lists from FUN_10010890/etc. + the weapon
table category field); (2) the config-panel EXACT colours/positions (the runtime hbreak capture — needs a
clean relaunch); the static-RE'd values applied so far are dims 128×20, config text white (placeholder),
category Cancel (100,100,205). Kevlar gameplay + status panel + HEV suppression + powerup/cashala = DONE.

## 2026-06-22 (cont. 69d/e) — buy-menu mechanism FULLY RE'd + reproduction begun (vgui)

User corrected my earlier guesses (top-level kevlar; "it's VGUI"; improvised draw/input) and set the
goal: RE everything, one at a time, until the buy menu is faithfully reproduced exactly like the original
(behaviour/interface on the newer SDK, not internal offsets). Did the full RE; reverted the improvised
cl_dll menu; began the faithful vgui reproduction. **All findings in `reference/MENUS.md`.**

**Mechanism RE (client.dll):**
- **It IS a custom `vgui::Panel`** (NOT the MOTD `gmsgVGUIMenu`, and NOT the stock `gmsgShowMenu`).
  `FUN_10010440` builds the buy-category panel via `vgui::Panel::Panel`; each entry is a `vgui::Button`
  (`FUN_100668f0`→`operator_new(0x14c)`→`FUN_10066b40`). Four panels on the client obj: `+0x2d84` config,
  `+0x2d88` buy-category, `+0x2d8c`, `+0x2d90`; shown/hidden via vtable+0x24 = setVisible.
- **Open/close** = `FUN_10011230(visible)`: copies saved→active config (`+0x434`→`+0x2c`), rebuilds
  (`FUN_10011160` item 1 = Use New/Last/Empty Config by state; `FUN_100129f0` = loadout cost/"Total"),
  setVisible the panels.
- **Entry model**: `FUN_10066a90(panel,idx,text,x,y,…,actionId,actionVal)` → text@entry+0x104, geometry
  via `FUN_10066bb0` (x@0xf4,y@0xf8,…), action@+0x144/+0x148. String table @`0x10092808` (config order:
  Empty/Use-New/Use-Last Config, 2 Buy, 3 Remove Item, 4 Reset, 5 Save, 6 Load, 7 Buy Random, 0 Cancel;
  buy-category @0x10092824: 1 Handguns,2 SMGs,3 Rifles,4 Shotguns,5 Other, +6 Player Accessory @0x10092848).
- **Hierarchy**: Config → 2 Buy → categories → 6 Player Accessory → Kevlar/Lasersight/Silencer
  (`cl_buykevlar`/`cl_laser`/`cl_lasercolor`; accessory selected-bit 0x10). Select by mouse-click OR bound
  number key; click-outside ⇒ "Use New Config".
- **Client buy-validation** `FUN_100124a0`: gates a buy on cash (CashBased off) + the **free-slots** resource
  `this+0x430` vs cap `0x51`(81) — the "Free Slots" HUD value + `GetSpeedBySlots` speed input.
- **Apply** `FUN_10012050` ("Use New Config"): validate (empty/cash/slots) → sort loadout → `ServerCmd`:
  `rst_cfg`, then per-item `tkw %i_%i`/`tka %i_%i`/`tki` (+ `weapon_%i`/`lasclr_%i`). = the existing server
  `ClientCommand` handlers (`reference/BUY.md`). Remaining tie-off: the exact kevlar item-queue token in the
  accessory branch.

**Reproduction (vgui, modelled on `cl_dll/vgui_teammenu.cpp`):**
- **Slice 1 DONE (committed):** `cl_dll/vgui_TSBuyMenu.{h,cpp}` = `CTSBuyMenuPanel` — the CONFIG menu as a
  `vgui::Panel` of `CommandButton`s with `setBoundKey(number)` + `CMenuHandler_StringCommand` → the server
  `menuselect` handler. Correct RE-grounded architecture (not the reverted hand-drawn HUD).
- **Slice 2 + 2c DONE (committed, IN-ENGINE VERIFIED):** owned in `TeamFortressViewport` (`CreateTSBuyMenu`),
  `ShowTSBuyMenu(show)` + `UpdateCursorState`, triggered from `CHudSpectate::MsgFunc_Spectator` (show in
  spectate-wait, hide on spawn). Retired the server `gmsgShowMenu` stand-in (`TSSendBuyMenu` no-op). **Verified
  by screenshot:** the vgui config menu (1 Use New Config … 7 Buy Random) draws during spectate-wait with the
  panel background; the old server text menu is gone; pressing bound key `1` fires `menuselect 1` + closes the
  menu. (The buttons reuse the server `TSMenuSelect` for config actions — a faithful bridge for the top level.)
- **Slice 3 DONE (committed) — buy-category submenu page.** RE'd from `FUN_10010440`: the category page
  (Handguns/SMGs/Rifles/Shotguns/Other + Player Accessory + `0 Cancel` at colour `(100,100,205)`) built as
  a toggled button page in `CTSBuyMenuPanel` with a `CTSMenuHandler_Page` action (config↔category nav).
  Compiles + renders. **KNOWN GAPS:** (a) the viewport routes number keys only to its *current command
  menu*, not this panel, so key-nav to the category page isn't wired (mouse-click on the vgui buttons is
  the path; cursor-state/UpdateCursorState needs confirming); (b) the per-category **weapon lists** + the
  **accessory** page (their builders decompile — next RE slice); (c) the config-panel exact colours/positions.
- **CONFIG-PANEL WALL — runtime inspect needed but NOT executable here.** The `+0x2d84` config panel is
  provably not statically RE-able (0 writes/allocs/sizers in 157K-line disasm; Ghidra mis-decompiles the
  region). The runtime-inspect path (run the ORIGINAL TS `client.dll` under wine+winedbg, break at the menu
  show, read the live `+0x2d84` panel's child positions/colours) is **blocked: no runnable original TS on
  this box** (no hl.exe / game install; `~/Projects/the-specialists` is stock-SDK source, not the menu code).
  Needs a HL+TS Windows install under wine first. Static-RE'd values applied so far: dimensions 128×20, the
  per-entry RGB colour *system*, the navigation; config items drawn white pending the inspect.
- **Slice 2d DONE (committed, VERIFIED) — RE-grounded styling.** First styled it wrong (used the SDK
  CommandButton's TFC orange box skin). RE'd the real draw: TS entries are plain `vgui::Button`
  (`FUN_10066b40` vtable = all imported vgui::Button methods, no paint override); colours set per-entry by
  `FUN_10011160` — **available = green (45,195,45), unavailable = red (195,45,45)** (button+0xfc/0x100). The
  orange was only `CommandButton::paintBackground` (boxes) + `paint()` forcing `Scheme::sc_primary1`.
  `CTSMenuButton` keeps CommandButton's bound-key/action but overrides `paint()` → TS green/red via
  `vgui::Button::paint`, `paintBackground()` → empty. Verified: green text on the faint panel, no orange.
- **NEXT slices (specified, not built):** (3) the **Buy** (`menuselect 2`) → weapon-category submenu + the
  **Player Accessory** submenu (Kevlar/Lasersight/Silencer via `cl_buykevlar`/`cl_laser`) — these are
  client-side nav panels (the 2nd/3rd of the 4 RE'd panels); the **per-category weapon lists** come from the
  category builders (still to decompile). (4) the client-side loadout-build + **apply** sequencer
  (rst_cfg/tkw/tki/tka, `FUN_10012050`) + the kevlar item-queue token. Mouse-click selection + cursor visibility
  to confirm in spectate. Build each as a compiled+verified increment.

---

## 2026-06-22 (cont. 69c) — kevlar GAMEPLAY reconstructed + buy-menu layout RE'd (two user corrections)

Continuing from the status-panel work, the user asked to reconstruct the real kevlar gameplay (not the
item_battery test crutch). RE'd from `ts_i386.so` and implemented; then the user caught two of my errors
and made me RE properly.

**Kevlar gameplay (committed):**
- `CBasePlayer::TakeArmor(int)` @0x83da4 — kevlar is a **binary flag** (`m_bHasKevlar`, orig player+0x718
  bit 0x10) + a 100 `armorvalue` marker; grant/remove + `weapons/items.wav`. SLICE: the orig also
  costs/refunds 10 **"free slots"** (player+0x548 — the HUD "Free Slots") and recomputes move speed via
  `GetSpeedBySlots`(@0x66bd4, `speed=210+freeslots*factor`)/`SetPlayerSpeed`(@0x83068, clamp 0..330 +
  pfnSetClientMaxspeed). That free-slots **encumbrance subsystem** is deferred (every item costs slots;
  the speed factors are runtime-init .data = 0.0 in file, statically unrecoverable).
- `CBasePlayer::TakeDamage` @0x7fa94 — armor is a **flat `armor_absorb` % damage REDUCTION, NOT a
  depleting pool**. Replaced the inherited HL depleting-armor block: if `armorvalue!=0` and dmgtype not in
  the **0x80040e4 bypass mask** (melee slash/club + fall + blast + radiation + no-armor bit → kevlar stops
  BULLETS only), `flDamage *= (1 - armor_absorb)`. New `CTSGameRules::m_flArmorAbsorb` (config-driven,
  default 0.5 — the orig parses `armor_absorb %f` from a server config into gamerules+0x588c; not a cvar).
- `CBasePlayer::AddItem(int)` @0x8440c — loadout item-grant (kevlar bit 0x10 → TakeArmor(100); grenade
  0x20 → team M61). `BuyArmament` @0x83f10 now routes item entries (the armament's 3rd field, renamed
  `gift`→`isItem`) → AddItem, weapons → BuyWeapon. Kevlar resets per-life in Spawn.
- Verified in xash: HUD shows "Kevlar" after granting via the loadout path (AddItem→TakeArmor→TSArmor→panel).
  Damage reduction verified by construction (faithful formula). Build clean, no coverage regressions.

**USER CORRECTION #1 — kevlar menu placement was INVENTED.** I'd added a top-level "Buy Kevlar" entry to
the (stand-in) buy menu by guessing — violating RE-before-code. RE'd the real menu (client.dll .data +
builders) → `reference/MENUS.md`. **USER CORRECTION #2 — it's NOT VGUI.** The buy menu is a **client-side
custom-drawn TEXT menu** (builders call `FUN_10066a90(panel,idx,text,x,y,…,scale)` positioned-text draw;
`FUN_10011160` = config-menu item 1), NOT `gmsgVGUIMenu` (the MOTD-style panel) and NOT the stock
`gmsgShowMenu`/`CHudMenu`. Hierarchy: **Config menu** (1 Use New/Last/Empty Config · 2 Buy · 3 Remove Item ·
4 Reset Config · 5 Save Config · 6 Load Config · 7 Buy Random · 0 Cancel) → **2 Buy** → categories (1 Handguns
· 2 SMGs · 3 Rifles · 4 Shotguns · 5 Other · 6 Player Accessory) → **6 Player Accessory** → **Kevlar** (toggle,
`cl_buykevlar`, accessory flag bit 0x10) / Lasersight / Silencer. So kevlar = **Buy → Player Accessory →
Kevlar**, never top-level. Reverted the invented entry + handler; fixed "7 Random"→"7 Buy Random".

**NEXT:** the faithful client buy-menu system (custom text-menu draw via FUN_10066a90 + per-category weapon
lists + each entry's send-action) is a Track B (cl_dll) reconstruction — the kevlar gameplay is ready, the
faithful UI to select it is deferred. Plus the free-slots encumbrance + speed penalty subsystem.

---

## 2026-06-22 (cont. 69b) — faithful TS status panel (posture figure + health-heart + Kevlar)

User feedback while verifying `powerup 3`: the "150" I screenshotted was the **stock HEV** health number,
NOT TS's custom HUD. TS replaces the stock health/battery entirely with a bottom-left **status panel**:
team name on top, a **posture figure** sprite, the **health number + a heart**, and a binary **"Kevlar"**
indicator (armor is kevlar-or-not, never a number — our old `CHudTSStatus` drew "Armor: N", wrong). The
figure reflects stance: standing / crouched / **prone** (prone via CTSStunt — +alt1 dive, double-jump
roll-to-prone, direction-dependent; e.g. double-jump-backward does NOT prone).

**RE (client.dll):** sprite-load block @0x10049d1f loads `sprites/player/{stand,crouched,prone,health,
kevlar,kungfu,movement,flag}.spr` (+ directional `divestep{b,c,t}.spr`) into slots stand=0x38 crouched=0x3c
prone=0x40 health=0x58 kevlar=0x5c. `"%i Kevlar  %i"` draw @0x1003808e. ("Free Slots : %i" @0x100929f8 is
the weapon/buy bar FUN_10011a60, a separate element — not this panel.)

**Reconstruction (`cl_dll/ts_status.{cpp,h}`):** rewrote VidInit (SPR_Load the 5 panel sprites + rects) and
Draw — team name (g_PlayerExtraInfo), posture figure (crouch via local player `curstate.usehull==1`; prone
via a new `g_iTSProne` extern hook, dormant until CTSStunt sets it), health + heart sprite, Kevlar drawn only
when `m_iArmor>0` (sprite if present, else "Kevlar" text). Missing sprites fall back to text (never crash).

**Verified in xash (screenshots vs the user's reference):** standing figure → 100 + heart; ducking → swaps to
`crouched.spr`; with real armor (sv_cheats 1; give item_suit; give item_battery) → the **"Kevlar"** label
appears below the health. Matches the reference. Scope chosen by the user ("Status panel HUD now") = stand/
crouch live, prone deferred to CTSStunt.

**Gotchas this session:** (1) TSHealth/TSArmor are sent server-side **on change only** — right after spawn the
panel shows 0 until the player fully enters play (settles to 100 a beat later; not a bug). (2) `give` needs
`sv_cheats 1`; **TS players lack the HEV suit**, so `item_battery` alone adds no armor — `give item_suit`
first (and TS's `sk_battery` may be 0). `item_battery` was only a TEST CRUTCH to force armorvalue>0; it is NOT
how TS grants kevlar.

**Stock-HEV suppression (cont.69b, committed):** user noted the stock HEV health number was still drawing over
the TS panel. The original `.so` registers **NO stock Health/Battery user messages** (only TSHealth/TSArmor —
no "Health"/"Battery" strings exist in ts_i386.so), so TS has no stock health HUD. Our inherited server still
sent gmsgHealth/gmsgBattery → the stock CHudHealth number/cross + CHudBattery drew. Fix: suppressed the stock
health-number block (`cl_dll/health.cpp`, `if(false && HasSuit())`) and the stock battery draw (`battery.cpp`,
early `return true`), keeping CHudHealth's directional damage feedback. Verified: bottom-left shows only the
TS panel. NB CHudHealth::Draw does NOT gate on HUD_ACTIVE (draws on HasSuit), so server-side message-suppression
alone wouldn't have stopped it — client suppression was required.

**KEVLAR mechanic RE'd (cont.69b — answering the user's Qs; NOT yet implemented server-side):**
- `CBasePlayer::TakeArmor(int)` @0x83da4: kevlar is a **bought binary flag** (bit 0x10 in player+0x718).
  Buying (arg>=0, flag clear): require cash(player+0x548)>=10, **deduct 10 cash**, set flag, play
  `weapons/items.wav`, then armorvalue = min(arg+armorvalue, cap). Selling (arg<0): refund 10 cash, clear flag,
  armorvalue=0. Also calls `GetSpeedBySlots`+`SetPlayerSpeed` → **kevlar imposes a move-speed penalty**.
- `CBasePlayer::TakeDamage` @0x7fa94: armor is **flat % damage PROTECTION, not a depleting pool**. If
  `armorvalue!=0` AND damage-type not in bypass mask 0x80040e4 AND valid player state:
  `damage -= armor_absorb * damage` (i.e. `damage *= (1 - armor_absorb)`), where `armor_absorb` =
  g_pGameRules+0x588c (cvar `armor_absorb`, clamped <=1.0). armorvalue is NOT decremented by hits. So the HUD
  showing kevlar BINARY ("Kevlar"/nothing) is correct. There's also `player/kevlarhit.wav` + "with kevlar"/
  "no kevlar" strings. **OPEN server work:** reconstruct TakeArmor + the TakeDamage armor_absorb block + the
  kevlar buy (cl_buykevlar → 10 cash) + the speed penalty (none implemented yet; our TakeDamage still inherits
  HL's depleting-armor model). Then the panel's kevlar reflects real gameplay.

NEXT for this area: the kevlar server mechanic above, wire kevlar into the buy/loadout, and the prone posture
once CTSStunt lands.

## 2026-06-22 (cont. 69) — `powerup` + `cashala` cheat commands wired (server cmds 2 missing → 0)

Closed the last two MISSING server console commands in the coverage harness — both turned out to be
**cheat/debug commands**, RE'd from `ts_i386.so` `ClientCommand` (objdump + GOT 0x165E10 resolution;
the harness's GOT-relative `lea→strcmp` reproduction enumerated the full dispatch).

**`cashala` (@0x9dd5d):** NOT the buy config-apply that `reference/BUY.md` (cont.68h) hypothesized — it's
a **cheat**: a float guard `flds 0.0 ; fcomps *g_flWeaponCheat` (the engine `sv_cheats` mirror, GOT
0x1dbc → `g_flWeaponCheat` @0x18f268) gates `GiveCash(player, 0x7530=30000)`. The `weapon_`/`%i` strstr+
sscanf right after it (@0x9dd98) is just the numeric `weapon_<N>` select transport (already handled).
Wired: `cashala` → `if (g_psv_cheats->value) player->GiveCash(30000)` (dlls/client.cpp). BUY.md corrected.

**`powerup <N>` (@0x9dafd):** the debug cheat that grants a powerup directly (the string is the `ts_powerup`
suffix @0xf76da, distinct from `usepowerup`). Gates: `Cmd_Argc()>1`, `CBasePlayer::IsAlive()` (vt+0xb4, via
VTABLES.md), a `pev+0x244` observer-state check (approximated by `!m_fInSpectateWait`). A jump table
(`-0xc8264(%ebx,%eax,4)`, base @0x9dbac) maps N→effect: **1→held SlowPause(8,5s), 5→held DblFire(0x10,5s),
default→held SlowMo(1,5s); 2→grenade weapon (PickUpWeapon tsweapon 24=M61), 3→health 150+regen, 4→MultiplyClips
(inf-clip) on the active CTSGun.** Held path guarded by `m_iPowerups==0` (orig 0x724 check) + same-type 5s
extend. Implemented `TSGrantPowerup(player, mask, dur)` in dlls/ts_powerup.cpp (mirrors PlayerAddPowerUp's
per-bit dispatch); client.cpp maps N→TS_PWUP_* and calls it.

**Verified:** `scripts/build.sh amd64 [client]` clean (glibc floor 2.14); `coverage.py all` → **server console
commands 2 missing → 0** (matched 18→20). **In-engine (xash3d-fwgs, listen server, screenshot):** spawned the
local player out of spectate-wait, `sv_cheats 1`; `cashala` → cash HUD **$30000**; `powerup 3` → health HUD
**150** — both accepted (no "Unknown command"). The original also fires a `PwUp` held-available HUD message on
the held path (deferred client work — still MISSING in user-messages). NEXT: the remaining coverage gaps are now
all client/HUD readers (PwUp/PwUpTime/TSState/TSSlowMo/Objective/ClipInfo/… — 25 MISSING user-messages) + the
TS events (coldcock/knife/katana/pump/grenade) + the 0%-method classes (CTSStunt/CTSSharedEmitter/CBulletManager).

## 2026-06-22 (cont. 68) — drop-in interface COVERAGE HARNESS + Ghidra tooling fixes

User asked for a coverage test proving our client+server are drop-in-compatible with the original on
the EXTERNAL contract (commands/cvars/messages/events/entities/exports — NOT internal C++ ABI, which
is explicitly out of scope per CLAUDE.md), to catch invented commands + HL leftovers + gaps and serve
as a RE roadmap. Also fix the recurring "Ghidra truncated" problem.

**Ghidra tooling (`scripts/ghidra/`):** `DisableNoReturn.java` (preScript — turns off the
"Non-Returning Functions - Discovered" analyzer that wrongly marks normal funcs noreturn and truncates
caller bodies at analysis time); `FixNoReturn.java` (postScript — clears stray noreturn flags except real
libc ones); `DecompAt.java` now auto-creates a function when Ghidra didn't define one (the cmd-handler
thunks) + clears noreturn on the target. `scripts/coverage/setup-ghidra.sh` rebuilds persistent projects
under `recovered/ghidra/{server,client}` (gitignored, survives reboot — /tmp is tmpfs here). NB the huge
`ClientCommand` still resists Ghidra flow analysis; the harness uses `objdump` for it (reliable).

**Coverage harness (`scripts/coverage/coverage.py`, pure objdump/nm/rz-bin — no Ghidra dep):** extracts
the interface manifest from original (ts_i386.so + client.dll + the original `ts/default.cfg`/`config.cfg`)
AND ours (hl.so + client.so + source), diffs → `reference/coverage/{original,ours}.json` + `REPORT.md`.
Categories DONE (matched/missing/extra): entity classes (FGD vs LINK_ENTITY_TO_CLASS, 140/8/141 — missing =
ts_emitter/ts_particle/ts_wind/info_player_team1-4/info_player_coop), client cldll exports (40/2/…),
server console commands (ClientCommand strcmp via GOT-resolved PIC: 8/12/24 — missing firemode/useitems/
respawn/jointeam/changeteam/motd/powerup/rst_cfg/cashalarm/tkw/tki/tka; extra = our tsbuy*/tsfiremode/
dropweapon/tscutwire/tsspawnflag stand-ins + HL leftovers give/+break/etc.), client commands (58/1/8),
client cvars (binary `pfnRegisterVariable` @ds:0x100d5b60, name+default: 93/14/21 + default mismatches),
user messages (client `pfnHookUserMsg` @ds:0x100d5b70: 51/25/7 — missing = the deferred HUD readers
PwUpTime/TSState/TSSlowMo/Objective/ClipInfo/…; extra = HL leftovers Flashlight/Weapons), events
(string-scan: 4/8/19 — missing TS coldcock/knife/katana/pump/grenade; extra = stock-HL crossbow/egon/
gauss/… we still precache), + a default.cfg input cross-check (2 unanswered: firemode, useitems).

**KEY CORRECTIONS from user ground truth (config files / running the original):**
- `buy` IS real (default.cfg binds b→buy; the client.dll `buy` cmd string @0x10095b34) — my earlier
  "no buy command" was wrong (I'd matched only the "1 Buy"/"2 Buy" menu entries). cont.66's buy is faithful.
- cvars must come from the BINARY's pfnRegisterVariable sites, NOT config.cfg — config.cfg holds only the
  archived (written-out) subset, so it falsely flagged stock cvars like zoom_sensitivity_ratio as "extra".

**Vtable + relocation tooling (cont.68b — user asked for richer C/C++ recovery via vtables/SDK
types/demangling):** Discovered the original .so DOES carry `.stab`/`.stabstr` — but ONLY for gcc-2.96's
own runtime units (libgcc2/tinfo/new/exception), no mod-class type info. BUT: 226 `__vt_<class>` vtable
symbols + RTTI exist, and the vtable slots (zero in the file) are filled by **R_386_32 relocations against
named method symbols** — so `readelf -W -r` yields each vtable+offset → exact method. Built
`scripts/coverage/extract_vtables.py` → `reference/coverage/vtables_original.json` + `VTABLES.md`: the
COMPLETE slot→method layout for all 226 classes (machine-confirmed `CTSGun` vtable+0x128 = `CTSGun::Deploy()`,
matching the cont.67 manual find; +0x130 Holster, +0x124 CanDeploy, etc.). Use: resolve any decompiled
`call [vptr+0xNN]` to the real method, and cross-check our virtual-override set per class. KEY INSIGHT: the
same **relocation map** resolves ALL .data pointers (cvar_t name/string structs, REG_USER_MSG name pointers,
function tables) that are zero-in-file — this unlocks the deferred coverage gaps (server cvars + message
sizes) and is the basis for the next Ghidra type-application pass (apply entvars_t C-struct + demangled
gcc2 signatures + vtable annotation). NB STABS is NOT useful for mod member offsets — those stay analytic;
but `entvars_t`/engine structs are pure C (ABI-stable offsets) so they CAN be applied verbatim.

**Scoreboard "WAITING" status for spectators (cont.68l, VERIFIED):** RE'd the original scoreboard
(client.dll FUN_1005db70) — the "class" column is a switch on the player's playerclass; **case 0xb (11)
→ "WAITING"** (and default→WAITING when the connected-flag is set). Reconstructed: server sets
`pev->playerclass = 11` (TS_SCORE_WAITING) while spectate-waiting (TSEnterSpectate, cleared to 0 in
TSLeaveSpectate) and broadcasts a ScoreInfo (new `CBasePlayer::TSSendScoreInfo`), re-asserted each
TSSpectateThink tick (a connect-time class-0 ScoreInfo was overwriting the spectate send — the bug behind
the first blank test); the TS team ScoreInfo (ts_teams.cpp) now sends pev->playerclass too. Client
(vgui_ScorePanel.cpp COLUMN_CLASS): playerclass 11 → "WAITING" UNCONDITIONALLY (before the TFC team gate
that was hiding it for teamless spectators). **Verified by screenshot** (tm_avana, spectating): the row
shows name + "WAITING", matching the user's reference (DM + teamplay). Note: AssignToTeam already sets the
per-team LOCKED model (ts_teams.cpp:165), so jointeam's model assignment is in place (the cont.68k
model-lock fidelity item).

**`jointeam` + `changeteam` wired + model↔team RE (cont.68k):** RE'd both handlers (objdump + GOT
resolution): `changeteam` pushes `gmsgVGUIMenu` → opens the team-select VGUI menu; `jointeam` strtols a
team number + calls a g_pGameRules team-change. Wired: `changeteam` → send gmsgVGUIMenu(MENU_TEAM=2);
`jointeam <N>` → `AssignToTeam(player, GetIndexedTeamName(N))` + respawn. Server-command missing 4→2
(remaining: `powerup`, `cashala` — both need RE). **MODEL↔TEAM finding (user Q):** the model is LOCKED —
`CHalfLifeMultiplay::ClientUserInfoChanged` (which TS inherits, NOT CHalfLifeTeamplay) resets a changed
`model` infokey back to the player's stored model (player+0xb48) via pfnSetClientKeyValue + says "Can't
change team to '%s'"/"HLDM models not allowed" (unless defaultteam off + a vtable IsValid check passes).
So team rides the model but you can't pick it by model-swap; team changes go through jointeam/changeteam,
model is force-locked to the assigned team. FIDELITY TODO: confirm our inherited ClientUserInfoChanged
matches this lock; the per-team locked-model assignment in jointeam is a SLICE (we set pev->team, not yet
the team model).

**`respawn` + `motd` wired (cont.68j):** RE'd both ClientCommand handlers (objdump). `motd` →
`SendMOTDToClient(edict)` (the protected CHalfLifeMultiplay method) → exposed via a public
`CTSGameRules::ResendMOTD` passthrough, called from the `motd` dispatch via `TSGameRules()`.
`respawn` → the original shows TSHudMessageOne "You have to wait until the next round." (red 195,45,45)
and StopObserver-respawns when allowed; reconstructed as `CBasePlayer::TSTryRespawn` — leave the
cont.66 spectate-gate (`TSLeaveSpectate`) if `gpGlobals->time >= m_flSpawnGateTime`, else the wait
message. Server-command missing 6→4 (remaining: changeteam/jointeam = team-select menu, powerup, cashala
= buy-apply — all need RE); 0 unanswered binds held.

**`useitems` wired → default.cfg cross-check now 0 unanswered (cont.68i):** `CTSGun::SwitchItems` +
`TSSwitchItems` + the `useitems` dispatch (default.cfg binds g). RE: the original SwitchItems body
(FUN_0009155c) is register-garbled in the decompile, but the ClientCommand gate (weapon id 16) + the
akimbo flag check + re-deploy identify it as the akimbo dual-wield swap. SLICE: our akimbo is a single
combined slot, so SwitchItems toggles the active barrel side (m_akimboSide) + re-deploys, gated on
iFireType AKIMBO/AKIMBO_ONLY; the full left/right items[] split is not reconstructed. Server-command
missing 7→6 (remaining: changeteam/jointeam/motd/powerup/respawn/cashala — team-menu/respawn/buy-apply);
**every default.cfg-bound command is now answered by our client or server (0 unanswered).**

**Buy/loadout-config command protocol RE'd + stand-ins renamed (cont.68h, `reference/BUY.md`):**
Decompiled the buy block of ClientCommand. Real commands + arg formats: `rst_cfg`→ResetArmament;
`tkw` arg `"%i_%i"`(type_count)→AddItemToArmament; `tki` arg `"%i"`→AddItemToArmament single; `tka`
arg `"%i_%i"`(caliber_count)→AddAmmoToArmament; `cutwire_<n>`/`drop_<n>` prefix commands;
`cashala…`→GiveCash + apply a config string parsing `weapon_%i` + `lasclr_%i`(laser colour). The
faithful buy flow = QUEUE (tkw/tki/tka) → APPLY (cashala config), not a direct per-weapon buy.
Renamed our stand-ins: tsarm→`tkw`, tsarmammo→`tka`, tsresetarmament→`rst_cfg`, tscutwire→`cutwire_<n>`
(prefix), added `tki`; kept tsbuy/tsbuyammo/tsbuyarmament as dev stand-ins (no 1:1 original — the
apply is cashala; honest "extra" in coverage). Server-command coverage: missing 11→7, extra 22→18.
Remaining missing = changeteam/jointeam/motd/powerup/respawn/useitems/cashala (team-menu/respawn +
the cashala apply-name, need RE). Buy UI (the client emitting these) still deferred.

**Acted on the coverage findings — 3 fidelity fixes (cont.68g):**
- **Health wire size 2→1** (the cont.68f finding): `REG_USER_MSG("Health",1)` + both `WRITE_SHORT`→`WRITE_BYTE`
  (player.cpp) + client `READ_SHORT`→`READ_BYTE` (health.cpp). Our build was self-consistent at 2 but the
  original uses 1 (health is byte-capped); now matches → drop-in safe. Harness confirms 0 size mismatches.
- **CTSGroundAmmo magsN under-give**: ours gave `magcount` rounds; the original gives `magcount * roundsPerMag`.
  Recovered the per-caliber rounds-per-mag table (s_ammo_info+0x10, runtime-built in compile_ammo_info — read
  via parsing its assignments, NOT the file which is zero; all 13 calibers cross-validated against +0x20 vs
  our kAmmoMax): `[30,12,8,30,10,30,20,5,15,17,5,20,5]`. Added `TSRoundsPerMag()` + multiply in WaitTouch.
- **ts_fog**: already correct — its original Spawn is `UTIL_Remove(this)`, so near/far/color are dead in both
  (no fix; added to SELF_REMOVING_ENTITIES so the harness stops flagging it).
- **Keyvalue-harness false positives fixed**: it now understands all 3 handling styles — literal FStrEq,
  `strncmp` PREFIX loops (mags/message/objptarget), and `keys[N]={...}` arrays (objmsg*) — + skips
  self-removing entities. Entity keyvalue coverage now **0 unhandled** (was 4 false-flagged entities).

**User-message WIRE SIZES added + a real bug caught (cont.68f):** `coverage.py server_usermsg_sizes_orig`
parses `LinkUserMessages__Fv` via objdump (`push $size; lea name(%ebx); call *0x12c` = pfnRegUserMsg slot)
→ 64 message sizes (Health=1, Damage=12, SayText=-1, RoundTime=2, WeaponInfo=6, WStatus=3, …); diffed vs our
REG_USER_MSG literal sizes. **Caught a real wire mismatch: `Health` is size 1 in the original, 2 in ours**
(a silent protocol corrupter — drop-in breaker; fix = match the original). Also fixed `fn_range` to skip
same-address symbol aliases (gcc2_compiled. shares LinkUserMessages' addr → was an empty range).

**Keyvalue-gap RE (cont.68f, acting on the harness findings):** `CTSGroundAmmo::KeyValue` (@0x92718) parses
`mags<N>` (strncmp "mags" + strtol suffix) → `m_mags[N] = magcount * get_ammo_info(N).roundsPerMag(+0x10)`,
plus `wduration`→respawn(+0xa8); our CTSGroundAmmo ignores the map mags = a real TODO. **`CTSFog::Spawn` is
just `UTIL_Remove(this)`** — the original deletes ts_fog on spawn, so near/far/color are intentionally
ignored: the harness's ts_fog keyvalue "gap" is a NON-issue (match by removing it).

**Tooling assessment — #1/#2/#4 (diminishing returns, NOT built):** native in-decompiler vtable typing is
superseded in practice by `vtcall.py` (resolves dispatch offsets→methods) for a heavy Ghidra-API lift;
dedicated RTTI/`__ti` hierarchy is redundant with the vtable data (inherited slots already name their base
class) and the gcc2 __ti layout resists clean automated parsing; spawnflag coverage can list the FGD
bit→label inventory but can't be verified as handled (we use raw bits/SF_ names, not FGD labels) so it's not
a real coverage check. Higher-value next work = act on findings (Health size, ts_groundammo mags, sk_*/
stock-HL event/message leftover cleanup).

**Ghidra type recovery extended (cont.68e) — entvars_t fields + MSVC return types:**
`ApplyTypes.java` now also (a) parses engine/progdefs.h to populate `entvars_t` with REAL fields
(extracts just the plain `typedef struct{}` blocks + a 4-type prelude, since the modern progdefs.h
has C++ `constexpr` the Ghidra C-parser rejects) → 668-byte entvars_t, so a typed `entvars_s *` param
resolves static `pev->origin`-style accesses; and (b) merges RETURN types from the MSVC exports
(signatures-mp.txt + signatures-client.txt: `public: RET __thiscall Class::Method(...)`), which the
gcc2 demangle omits → 691 return types parsed, 164 applied (complete return+param prototypes on the
exported subset). Verified: entvars_t 668B, params typed entvars_s*. (Funcs using the runtime
gEntvarsDescription offset-table still show computed offsets — expected.) setup-ghidra.sh auto-applies.

**Coverage dimensions added (cont.68d) — entity keyvalues + method completeness:**
- **Entity keyvalue coverage** (`coverage.py`): per implemented FGD entity, which declared map keys NO
  `KeyValue()` handles (engine-auto keys = entvars_t fields + stock/compile-tool keys excluded). Surfaces 4
  real behavioral gaps: `ts_fog` ignores far/near, `ts_groundammo` ignores mags1..11, `ts_objective_manager`
  ignores messagea-d/objptargeta-h, `ts_objective_ptr` ignores objmsg*. = internal-state replication TODOs.
- **Method completeness** (`scripts/coverage/reconstruction.py` → `RECONSTRUCTION_COVERAGE.md`): per TS class,
  methods in the original (signatures-gcc2) vs implemented in our hl.so (nm|c++filt). **60 classes, 140/368
  methods (38%)**, least-complete first: CTSStunt 0/25, CTSSharedEmitter 0/11, CBulletManager 0/7, the
  particle/emitter/wind + per-category weapon-factory classes 0% (some intentional — CTSGun subsumes the leaf
  CTSPistolWeapon/etc.). A completeness ROADMAP (not an ABI match). Also renamed tsfiremode→firemode,
  dropweapon→drop (real names, default.cfg-confirmed); committed cont.67 weapon-switch + all tooling.

**Reconstruction tooling — the 3 improvements built + validated (cont.68c):**
- **#1 typed decompiles — `scripts/ghidra/ApplyTypes.java`** (postScript): applies the recovered gcc2
  signatures (`reference/signatures-gcc2.map`) as REAL Ghidra prototypes → 2570 functions typed; the `this`
  pointer + call args are typed (`Deploy__6CTSGun(CTSGun *this)` not `int param_1`). Conservative type model
  (named empty structs for classes — NO faked member layout, since gcc2 offsets differ from our Itanium SDK
  and there's no mod debug info; members still show as offsets). Auto-run by `setup-ghidra.sh` after import.
- **#2 virtual-dispatch resolution — `scripts/coverage/vtcall.py`**: `vtcall.py CTSGun 0x128` → `CTSGun::Deploy()`;
  `--file <decomp.c> CTSGun` annotates every `+0xNN))(` virtual call with its method (66 in CTSGun.c) — turns
  blind dispatch into named calls (SendWeaponAnim/Deploy/Precache/…). Uses vtables_original.json.
- **#3 server cvars — closed the coverage gap** (`coverage.py server_cvars_orig`): scans ts_i386.so .data for
  `cvar_t {name,default,...}` structs (the .rodata string ptrs are R_386_RELATIVE addends stored in-file) →
  41 matched / 5 missing (deadback/deadsitting/sv_alltalk/voice_serverdebug/weaponstay) / 258 EXTRA, of which
  **252 are stock-HL `sk_*` skill cvars** (monster/weapon damage tables) = a big HL-leftover cleanup target
  (TS has no HL monsters). Report collapses the sk_* family for readability. Default-mismatch detection too.
Remaining harness gap: user-message wire SIZES (pfnRegUserMsg site scan); cvar/engine-call USAGE tracing.

**Engine constants recovered:** ts_i386.so `_GLOBAL_OFFSET_TABLE_`=0x165e10 (PIC string-ref base);
client.dll cl_enginefuncs: pfnAddCommand=ds:0x100d5b6c, pfnRegisterVariable=ds:0x100d5b60,
pfnHookUserMsg=ds:0x100d5b70.

**Honest completeness:** DONE = the full by-NAME swap contract above. GAPS = server-side cvars + message
SIZES (ride GOT-relocated ptrs / cvar_t structs in the PIE — need R_386_RELATIVE reloc parsing); cvar/
engine-call USAGE tracing (semantic xref); internal C++ ABI/methods (OUT of scope — engine never sees it).
Spot-check Qs sent to the user (running the original): does `weapon_<N>` switch in console (validates the
cont.67 transport); what do tkw/tki/tka/rst_cfg/cashalarm do; do give/impulse 101 work (HL-leftover check);
is `buy` usable while alive; is cl_bob default really 4. NOT committed yet.

## 2026-06-22 (cont. 67) — weapon-switch fidelity: drop invented tsnext/prevweapon → faithful weapon_<N> + lastinv (RE-grounded, in-engine verified)

User picked "weapon-switch fidelity" off the cont.66 known-unfaithful list. RE-FIRST (per
`check-re-on-feedback`) before any code; findings written to `reference/WEAPON_SELECT.md`.

**RE (client.dll cursor + ts_i386.so commit):**
- Imported client.dll + ts_i386.so into persistent Ghidra projects (`/tmp/ghidra-client`,
  `/tmp/ghidra-server`); DecompAt/DecompRefs by address. `scripts/ghidra/DecompAt.java` only
  resolves addresses Ghidra defined as functions — small thunks (the cmd handlers) needed
  raw `objdump -d -M intel` instead.
- **No `tsnextweapon`/`tsprevweapon`** anywhere (client cmd table nor server ClientCommand)
  — confirmed invented. Client cmds: invnext/invprev/slot1..10/+attack/+attack2/
  cancelselect/menuselect. Real **server** ClientCommand set (PIC base ebx=0x165E10, exact
  strcmp list): say, say_team, fov, use, respawn, powerup, cashalarm?, rst_cfg?, tkw, tki,
  tka, cutwire_*, drop_*, **drop**, **usepowerup**, **firemode**, **useitems**, **lastinv**,
  changeteam, jointeam, motd, fullupdate.
- Client cursor `FUN_1004f500(this,dir)` (invnext dir=1 @0x1004fce0 / invprev dir=0
  @0x1004fd40, both rate-limited): seeds (row 0x740, col 0x744) from the active weapon, steps
  the 17×5 grid skipping empties/akimbo-pairs + wraps; then if cvar **`hud_fastswitch`!=0**
  COMMITs by formatting **`"weapon_%i"`** (@0x10095db8) and sending it to the server (else
  the bar stays open at 0x73c for a +attack/slotN commit). → **commit transport =
  `weapon_<N>` console cmd** (numeric tsweapon_t).
- Server `CTSGun::SelectWeapon(type)` @0x80fb8 = HolsterWeapon(@0x85698) + set m_active
  (only if owned+valid); the **caller** (ClientCommand block @0xae433) does `if changed
  gun->Deploy()` (vtable+0x128). So switch = holster(old)→set→deploy(new). `lastinv` block
  saves player+0xa1c=current & re-selects the prior type. `CBasePlayer::SelectItem(char*)`
  is the stock by-CLASSNAME search — NOT the numeric path.

**Implemented + verified:**
- Server (`dlls/ts_gun.cpp`/`ts_weapons.h`/`client.cpp`): removed TSCycleWeapon/Prev +
  tsnext/prevweapon dispatch; `SelectSlot` now holster→deploy (cancels reload, records
  `m_lastActive`); `weapon_<N>` dispatch (strncmp+digit guard) → `TSSelectWeaponType` →
  FindSlot→SelectSlot; `lastinv` → `TSLastInv`.
- Client (`cl_dll/ts_weaponbar.{h,cpp}`/`ammo.cpp`): `CHudTSWeaponBar::CycleType(fNext)`
  walks owned weapons in the bar's column-major display order; invnext/invprev emit
  `ServerCmd("weapon_%d\n")`.
- Build amd64 all = clean (glibc 2.14); `nm|c++filt` shows TSSelectWeaponType/TSLastInv;
  no `tsnext/prevweapon` strings in either .so; `weapon_%d` in client.so. **In xash3d-fwgs
  (tm_avana), spawned in + mouse-wheel: rifle→akimbo pistols (invnext ×2), →single pistol
  (invprev), no crash, no "Unknown command".** Screenshots `/tmp/ws_{spawn,next,prev}.png`.

**Buy-menu availability (the cont.66 open Q):** CORRECTED by user ground truth — **`buy` IS
real** (original `ts/default.cfg` binds `b`→`buy`, also firemode/useitems/drop/usepowerup;
the `buy` string is in client.dll @0x10095b34 — I'd wrongly matched only the "1 Buy"/"2 Buy"
menu entries). cont.66's `buy` reopen is faithful. The menu is server-driven (gmsgShowMenu);
the exact dead/alive availability gate is a TODO for the coverage harness + a targeted RE of
the `buy` handler.

**Deferred (noted):** hud_fastswitch=0 open-bar + on-bar select highlight + literal (row,col)
grid cursor (we commit immediately + walk the compact owned-set). Separate fronts surfaced:
rename `tsfiremode`→`firemode`, `dropweapon`→`drop`; `usepowerup` IS real (HANDOFF's
"invented" note is wrong); buy cmds = tkw/tki/tka/rst_cfg/cashalarm.

## 2026-06-22 (cont. 66) — TS join/spectate/buy-config/weapon-select flow (IN PROGRESS; some unfaithful stand-ins to fix)

User asked for the faithful join flow: a joining player does NOT spawn straight into play — they enter
free-look spectate, see a countdown → "Press fire to play", and pick a loadout from a buy/config menu first.
Plan: `~/.claude/plans/replicated-discovering-aho.md`. Used 3 Explore agents to map it (server spectator,
client buy-menu/countdown, buy backend + wiring patterns).

**DONE + verified (commits 0d4f2d0, 60a8c44, d1b0974, a1a8149):**
- **Spectate-gate (server):** `CBasePlayer::TSEnterSpectate` (StartObserver/OBS_ROAMING + respawn gate
  `respawntime` clamp 3..30 + ResetArmament), called at the end of `Spawn` when `m_fInSpectateWait`;
  `ClientPutInServer` sets the flag pre-Spawn; `CTSGameRules::PlayerSpawn` skips the loadout while waiting.
  `TSSpectateThink` (PreThink) drives the countdown; **+attack while !configChosen = select item 1,
  +attack after = `TSLeaveSpectate` → respawn into play**. Registered gmsg `Spectator`(2)/`VGUIMenu`(1).
- **Free-look HUD (client `cl_dll/ts_spectate.{cpp,h}`):** centre countdown → "Press fire to play" + the
  free-look help line; reads `Spectator`; `__MsgFunc_Spectator` repointed from the TFC viewport to it.
- **Buy/config menu — FAITHFUL mechanism:** the compact **`gmsgShowMenu` + `menuselect`** list (NOT a VGUI
  panel — I first wrongly built a VGUI MOTD-style panel; user corrected). RE'd from `client.dll FUN_10011160`
  (item 1 = Use New/Last/Empty Config; available items green). `TSSendBuyMenu`/`TSMenuSelect`/`TSCloseMenu`
  (`dlls/player.cpp`); client `CHudMenu` draws it, added `\g` green code (`cl_dll/menu.cpp`); resent each
  spectate tick (the first send can beat the client HUD). `buy` command reopens it (spectate-only). Verified:
  compact green-coded list matches the user's reference; click → "Using new config." + closes.
- **Weapon-bar layout (client `cl_dll/ts_weaponbar.cpp`):** RE'd from `FUN_1004e030` — 5 columns
  `x=col*0x80-0x78`, header panel + slot number (alpha 100 active/50 idle), weapons stacked 0x30/row with
  sprite icons (red when clip empty), active weapon name+ammo to the right. Verified vs reference.

**KNOWN-UNFAITHFUL / OPEN — the user set a hard REQUIREMENT (memory `check-re-on-feedback`): RE before coding,
and verify every console command exists in the original (it caught me inventing commands):**
- **Weapon switching uses INVENTED `tsnextweapon`/`tsprevweapon`** (client invnext/invprev forward to them,
  `cl_dll/ammo.cpp`). NOT in the original. Faithful: invnext/invprev move a **select cursor** (the `+0x7c0`
  select-active object in `FUN_1004d3f0`), **+attack commits**; switching plays **holster→deploy** (SelectSlot
  currently Deploys instantly).
- **Timed-powerup activation on invented `usepowerup`** — should be **double-tap `+use`(IN_USE) in PreThink**.
- **`tsbuy`/`tsarm`/`tsbuyarmament`/`tsresetarmament`/`tsbuyrandom`** are internal stand-ins; verify vs the
  original (BuyWeapon/BuyArmament are real functions; command names need RE).
- **Not built:** bar faint-when-idle fade; active **clip/maxclip "8/8"**; **powerup-timer HUD** (Slow Motion/
  Double Fire Rate green text above weapons, from `PwUpTime`); **kung-fu/super-jump icons** (centre-bottom);
  buy-menu **cursor + click-on-entries**; the **"2 Buy" weapon-category submenu**; menu **bg box**.
- **Open Q for user:** buy menu the whole dead-spectate, or only the join/respawn press-fire wait?

**Verification loop note (gotcha):** reading the Serena `ledger` memory returns ONE long single-line JSON
(`{result: "..."}`) that overflows the read limit — extract with `jq -r '.result' <savedfile> | head -c N`
or read the saved tool-result file in chunks; don't try to Read it whole.

## 2026-06-22 (cont. 65) — CTSAnimation: reconstruct 3rd-person body/gait (fix the always-idle player), VERIFIED

Fixed the **biggest open item** (the player 3rd-person model was always idle — no walk/run/fire gait).

**Root cause (decomp-grounded):** the original `CBasePlayer::SetAnimation(PLAYER_ANIM)` is a **no-op stub**
(`@0xd8f50`, 21 bytes) — TS drives ALL body animation through `CTSAnimation::Animate` (`@0x6abd0`, called
from `PostThink` with activity 1=move / 6=idle, from `Duck`=5/`Jump`=4, and the weapon fire/reload/death
paths). Our build still ran the STOCK SDK SetAnimation, which looks up `"ref_aim_<ext>"`/`ACT_*` sequences
the TS `player.mdl` does NOT define (it ships `idle_/aim_/shoot_<grip>`, `run2`/`walk2`/`crawl`, `crouch_*`,
`standard_jump`, `die_*`, …; dumped all 171 seq names from `~/Containers/hlds/mods/ts/models/player.mdl`).
So every lookup returned -1 → sequence 0 (`empty`) → permanent idle.

**Reconstruction (`dlls/ts_animation.{h,cpp}`, `CTSAnimation m_TSAnim` embedded on CBasePlayer):** by
INTENT from the ~6.7KB Animate state machine (behaviour + the model/client contract is the target, not the
gcc2 byte layout — the decompile is hostile: pointer-to-member string-build loops + stack artifacts).
- `pev->sequence` (torso) = `shoot_`/`relo_`/`draw_`/`aim_`/`idle_` + grip, with `crouch_`/`duck_` variants;
  `standard_jump`/`jump_<dir>_<grip>`; `goduck`; `die_spin`; `grenade`. Grip = `m_szAnimExtention`
  (onehanded/twohanded/akimbo), already set by `CTSGun::Deploy` (the original's player+0x2d6).
- `pev->gaitsequence` (legs) = `crawl` (ducking+moving) / `run2` (>135 u/s) / `walk2` / 0. The client
  `StudioProcessGait` plays gaitsequence blended under the torso sequence (verified it reads the
  server-networked value, doesn't pick walk/run itself).
- aim-hold timer (`m_flAimTimer`=2.0 on fire/reload) protects the just-set shoot/relo torso pose from being
  overwritten by the per-frame idle path — mirrors the original's `param_1[3]` gate (`@0x6cf..`).

**Wiring:** `CBasePlayer::SetAnimation` rewritten to route the PLAYER_ANIM call sites →
`m_TSAnim.Animate` (JUMP/SUPERJUMP→4, DIE→7, ATTACK1→0, WALK→1, IDLE→6); `CTSGun` drives
reload(`relo_`,0x33)/deploy(`draw_`,0x34)/grenade(`grenade`,0x35) directly; `PreThink` decays the timers;
`Spawn` calls `Init()`. (Fire already routed via the existing `SetAnimation(PLAYER_ATTACK1)` call sites.)

**Deferred (CTSStunt-gated — CTSStunt is NOT ported, so these activity codes never fire):** dives
(0x28-0x2b,0x2e-0x30), cartwheels/walljump (0x31,0x32,0x37,0x38), morfy-jump (0x39), rolls/bullet-dance
(0x36), close-combat blends. The directional-aim 9-way movement blend (BlendAnimationsByDir) + death-by-
direction variants are also a refinement (we use die_spin). The `CTSAnimation::Init` string tables for the
deferred stunt names are intentionally not reproduced.

**Verified (xash3d-fwgs thirdperson, this box's GPU, `+thirdperson` on tm_avana; used xdotool to drive
keydown w / mousedown 1 / keydown ctrl + `import` screenshots):** armed standing idle pose (was broken),
walk/run **leg gait cycling** across frames, distinct **crouch** stance. No crash, no sequence-out-of-range
in the dev log. Server + client both build clean (amd64), `nm|c++filt` shows the 6 CTSAnimation methods.
Committed `2c44837`.

**NEXT:** fire/aim torso pose is hard to confirm from the behind-camera (works structurally; a front view or
rcbot would confirm); directional-aim strafe blend (BlendAnimationsByDir) for proper 8-way movement;
death-direction variants; then `CTSStunt` (which unlocks the deferred Animate branches) + full pmove
time-dilation. See `reference/ENGINE_INTERFACE.md` "notable gaps".

## 2026-06-22 (cont. 64) — playtest fixes: fire-anim regression, 3rd-person camera, coldcock sound

User playtested (cont.61–63 build) and reported issues; fixed three:
- **Viewmodel fire animation REGRESSION (I caused it in cont.56).** Moving the anim to the predicted
  event's `EV_WeaponAnimation` broke it — that doesn't stick because CTSGun isn't client-predicted.
  Restored the server-driven `SendWeaponAnim(iFireAnim)` + `EF_MUZZLEFLASH` in `CTSGun::StandardFire`
  (the working cont.49 path); the event now only does fire sound + brass shell + viewmodel muzzleflash
  (removed EV_WeaponAnimation from EV_TSFire).
- **3rd-person camera = side profile, should be behind.** `cam_idealyaw` default was 90 (= camAngles[YAW]
  = 90 + viewYaw → side). Changed to **0** (directly behind, looking the player's direction). VERIFIED by
  screenshot: player now seen from behind facing into the room.
- **+alt2 coldcock played a KNIFE sound (wrong).** Swapped the knife_hitbody/miss sounds for
  `player/kungfuhit.wav` (a melee-bash impact) on hit, silent on miss.

**Still open from the playtest (NOT yet fixed):**
- **Player 3rd-person model is always idle** (no walk/run/fire gait) — this is the `CTSAnimation` gait
  subsystem (PostThink, NOT ported; cont.60). Big remaining piece (movement/anim, client-entangled).
- **+alt1 does nothing** — expected; it's an xash built-in + the silencer is UNMAPPED (VERIFICATION §B).
- damagemult untested (no enemy; needs rcbot in xash).

## 2026-06-22 (cont. 63c) — wire `initial_cash` cvar → starting cash (cash HUD lights up), VERIFIED

`CTSGameRules::PlayerSpawn` (`dlls/ts_teams.cpp`) now gives `initial_cash` (cvar) via `GiveCash` once per
fresh life (`if m_iCash == 0`), so the player starts with cash. **Verified WITH a screenshot:** launched
`+initial_cash 5000` → the bottom-left cash HUD shows "$5000" — confirming the whole chain (cvar →
GiveCash → m_iCash → TSCash message → CHudTSStatus reader). Ties the cvar wiring to a visible HUD element.
`cashforwin`/`cashforlose` (round-end award via TeamWin) still TODO. API_SURFACE §6c: initial_cash ★wired.

## 2026-06-22 (cont. 63b) — wire `damagemult` cvar → weapon damage scale

`damagemult` cvar (default 0.0 = off → 1x; >0 scales) now multiplies all TS weapon damage via a
`TSDamageMult()` helper (`dlls/ts_gun.cpp`, `CVAR_GET_FLOAT`) applied at all 5 damage points: StandardFire/
ShotgunFire (FireBulletsPlayer), AkimboFire, MeleeAttack, ColdCock, GrenadeThrow. Builds clean; the
cvar-read pattern is proven (roundtime/slowmatch). Combat scaling itself needs a playtest shot to confirm
on-screen (combat-gated). API_SURFACE §6c: damagemult ★wired.

## 2026-06-22 (cont. 63) — pfnGetGameDescription → "The Specialists (mode)"

`CTSGameRules::GetGameDescription` (new override, `dlls/ts_teams.cpp`) returns the mod name shown in the
server browser, per mode (recovered .so strings): "The Specialists (Teamplay)" if IsTeamMode, "(The One)"
if mp_theonemode, "(LMS)" if lastmanstanding, else "(DM)". Was returning the SDK "Half-Life". Verified:
strings present in hl.so. ENGINE_INTERFACE.md: pfnGetGameDescription ✅. Remaining interface gaps now:
full pmove time-dilation (gravity/jumps/anim), PlayerPostThink (CTSAnimation/CTSStunt), HUD_PostRunCmd
(client weapon prediction), TS save/restore field data.

## 2026-06-22 (cont. 62) — VISIBLE slow-motion movement (the marquee TS effect), VERIFIED

Wired the biggest tracked interface gap: slow-motion is now VISIBLE. `CBasePlayer::PreThink` scales the
speed cap by the applied slow factor — `pev->maxspeed = 320 * m_flSlowMotionGo` when slowed (`m_flSlowMotionGo`
in (0,1)), restoring to the engine default (0) on exit (tracked by new `m_bSlowMoveApplied`). pev->maxspeed
flows to pmove via clientdata (client.cpp:1944), so a slowed player physically moves slower. This makes the
whole complete-but-invisible server slow state (GoSlow/InSlow/GlobalSlowMotion2/slowmatch/CTSSlowMotionPoint/
powerup slow) actually do something on screen. **Verified WITHOUT input:** `+slowmatch 0.5` → log probe
"[TS] slow-move: maxspeed 160 (scale 0.50)" (320→160); probe removed, rebuilt clean.
- SLICE: full pmove time-dilation (gravity/jump arcs/animation rate) needs the slow value threaded into
  client-predicted pm_shared — the complete effect. This is the server-side speed portion.
- ENGINE_INTERFACE.md: pfnPM_Move gap ❌→🟡.

Also this session (cont.61): thirdperson MP-gate fix; ENGINE_INTERFACE.md (full callback tracking).

## 2026-06-22 (cont. 61) — fix thirdperson (MP gate) + track the FULL engine↔mod interface (the goal)

- **`thirdperson` FIXED + VERIFIED.** The stock HL client's `CAM_ToThirdPerson` (`cl_dll/in_camera.cpp`)
  hard-returns `if (GetMaxClients() > 1)` ("no thirdperson in multiplayer") — TS runs as MP, so the
  command did nothing (the user's report). TS permits third-person (it's a real feature); removed the gate.
  Verified by launching `+thirdperson` → screenshot shows the player model from behind.
- **FULL interface tracking — `reference/ENGINE_INTERFACE.md` (new), per the mission goal "all engine calls
  and callbacks."** Enumerated + status-tracked every engine↔mod entry point: (1) engine→server
  `DLL_FUNCTIONS` (~46 callbacks, from cbase.cpp) + NEW_DLL_FUNCTIONS; (2) engine→client cldll exports
  (~45, from cdll_int.cpp); (3) the by-name protocol (→ API_SURFACE/HUD_MESSAGES); (4) mod→engine calls.
  Each marked ✅faithful / 🟡partial / ⬜stock-inherited / ❌missing. **The real remaining interface GAPS
  (highlighted there):** `pfnPM_Move`+`HUD_PlayerMove` = the VISIBLE slow-motion time-dilation (scale pmove
  frametime by the slow field — nothing reads the complete server slow state in movement; biggest gap),
  `pfnPlayerPostThink` = CTSAnimation+CTSStunt, `HUD_PostRunCmd` = client CTSGun prediction,
  `pfnGetGameDescription` = "The Specialists", TS save/restore field data.

## 2026-06-22 (cont. 60) — status tracking: think behaviour, cvar/cmd impl status, playtest checklist + silencer

Answered the user's tracking questions with durable docs.
- **Think behaviour — PARTIAL (not all ported).** Original `CBasePlayer::PostThink` calls
  `CTSAnimation::Animate` (gait/anim subsystem, 14 funcs), `CTSStunt::GoRoll` (stunts, 25 funcs) and
  `CheckPowerups` — **none ported**. We have PreThink→`UpdatePowerUpState`, `UpdateClientData`(+TS HUD),
  and `CTSGameRules::Think` (deads/round/powerups/slowmatch). The CTSAnimation + CTSStunt subsystems are
  the remaining per-tick gap (movement/animation, client-entangled). Recorded in `reference/VERIFICATION.md` §C.
- **cvar/cmd implementation status — now TRACKED** in `reference/API_SURFACE.md` §6c: a R(egistered)/read/
  ★wired table for every TS cvar + command. Wired so far: roundtime, slowmatch, hud_r/g/b, +alt2/coldcock,
  + the server console gameplay cmds. The rest are R-only; `damagemult` flagged as the next wiring candidate.
- **Playtest checklist + silencer tracking — `reference/VERIFICATION.md` (new).** §A: 9-item playtest
  checklist with exact steps (HUD ×3, round clock, predicted fire, fire-sound pitch, coldcock, slowmatch,
  no-spam) for the user to confirm. §B: the silencer/`+alt1` assembly findings + what to implement + how to
  verify (it's UNMAPPED; the `-sil.wav` files exist; the install state rides the WeaponInfo flags byte +
  player+0x798 mask). §C: the think-status above.

No code this entry — documentation/assessment.

## 2026-06-22 (cont. 59) — wire the `slowmatch` cvar into gameplay (global slow-motion match), VERIFIED

Wired a registered cvar into actual behaviour (the user asked for a verifiable cvar→gameplay hookup).
`CTSGameRules::Think` now reads `slowmatch` (cont.54 cvar, default 1.0): a value in (0,1) runs the WHOLE
match in global slow-motion at that scale via the already-tested `GlobalSlowMotion2(sm,0)` (self-throttled,
re-applied each Think); returning to >=1.0 restores once (`GlobalSlowMotion2(0,0)`), tracked by new
`m_bSlowMatchActive`. **Verified WITHOUT in-game input:** launched xash with `+slowmatch 0.5` → log printed
the (temporary) probe "[TS] SlowMatch ON: scale 0.50", confirming the cvar is read + applied; probe removed,
rebuilt clean. (The slow path itself — GlobalSlowMotion2 → GoSlow/InSlow — was runtime-verified earlier,
cont.15/33.) This is the launch-arg verification pattern for cvar→gameplay wiring.

**Checkpoint still stands:** the remaining input-gated gameplay (the +alt1 silencer is also UNMAPPED, cont.58;
stunts; buy menu; true client prediction) needs a USER PLAYTEST to verify. Other mapped+verifiable cvar
wirings available next: `damagemult` (damage scale), `cashforwin`/`cashforlose`/`initial_cash` (the economy).

---

## 2026-06-22 (cont. 58) — finding: SecondaryAttack = IN_ATTACK2 dispatch; +alt1 silencer NOT mapped; PLAYTEST checkpoint

Scoped the `+alt1` silencer/scope toggle (next gameplay item). Findings, no code (avoided speculation):
- `CTSGun::SecondaryAttack @0x717a8` is the **IN_ATTACK2 fire-type dispatcher**, NOT the silencer: branches
  on the active weapon's fire-type (s_weapon_info+0x348) — no active item → `CCombatAttack` (unarmed combat),
  type 2 → `GrenadeFire`, type 3 → (knife branch), etc. A real recoverable dispatch, but input-gated gameplay.
- `CTSGun::AlternateAttack @0x81ba8` = ColdCock (done cont.57); `Zoom`/`Unzoom` are forwarders.
- The **`+alt1` silencer/scope toggle handler is not cleanly located** — no server-side "silenc"/attachment
  name strings to anchor on (only client.dll has "/HUD/Silencer"); the install logic is diffuse. Recovering
  it would be speculative RE, and like all the remaining gameplay it's INPUT-GATED (can't self-verify here).

**Checkpoint reached:** the remaining work is all input-gated gameplay (silencer, stunts, buy menu, true
client prediction) that can't be self-verified without a playtest. Recommended a USER PLAYTEST of the
cont.51–57 build (HUD 3 regions, fire event, coldcock, cvars/round timer) to surface real issues before
stacking more unverifiable features. Did not implement +alt1 (unmapped + unverifiable).

---

## 2026-06-22 (cont. 57) — gameplay: +alt2 coldcock (pistol-whip)

First command-gameplay behaviour wired. `CTSGun::AlternateAttack` (@0x81ba8) + `CTSGun::ColdCock`
(@0x71dd0, calls CBasePlayer::ColdCockAttack@0x86610 in the .so) reconstructed in `dlls/ts_gun.cpp`:
AlternateAttack coldcocks when the held weapon has a round loaded (the .so gates on active clip > 0);
ColdCock is a short hull-trace melee bash (40u forward, 25 club damage, knife hit/miss sounds, 0.5s
cadence, 3rd-person attack anim). Free fn `TSAlternateAttack(player)` (declared ts_weapons.h) finds the
held gun + fires it. Server `ClientCommand` "+alt2" → `TSAlternateAttack` (`dlls/client.cpp`); the client
already forwards +alt2 (cont.56). Precached the two knife sounds in `CTSGun::Precache`.
- **Verified:** builds, loads + runs on tm_avana, sounds precache, no crash/unknown-command for +alt2.
  **NOT functionally confirmed** (can't press +alt2 without interactive input) — needs a user playtest.
  SLICE: the faithful coldcock viewmodel anim rides events/coldcock.sc (not reconstructed); we drive the
  3rd-person anim + sound server-side.
- **Leftover config warnings (minor, not chased):** the TS config sets engine `s_*` sound cvars (xash
  names them differently), `sv_language`/`sv_voicecodec` (engine), and `weaponrestriction`/`weaponstay`
  (the latter is the mp_-prefix-vs-bare-name mismatch — the base SDK registers the struct but maybe under
  a different name string). Low value.

---

## 2026-06-22 (cont. 56) — Phase 5 (partial): faithful events/fire.sc → EV_TSFire (sound/flash/anim/shell)

Moved weapon firing onto the original's event path. **Server** (`dlls/ts_gun.cpp`): `CTSGun` precaches
`m_usFire = PRECACHE_EVENT(1,"events/fire.sc")`; `StandardFire` now `PLAYBACK_EVENT_FULL(0, player, m_usFire,
…, fparam1/2=spread, iparam1=m_weaponType, bparam1=(clip==0))` instead of the server `EMIT_SOUND` +
`SendWeaponAnim` slice (cont.49) — bullets still fire server-side. **Client** (`cl_dll/ev_ts.cpp`, hooked in
`hl_events.cpp` Game_HookEvents): `EV_TSFire` reads weapontype → viewmodel muzzleflash + "shoot" anim
(local player) + brass shell eject + the per-weapon fire sound (client sound/anim tables mirroring
`ts_weapons.cpp`). Adds the viewmodel muzzleflash + shell the server-driven slice lacked.
- **Event is SERVER-triggered, not yet client-PREDICTED** — true prediction needs CTSGun client-side
  weapon prediction (HUD_PostRunCmd), a separate subsystem. This is the event-layer prerequisite for it.
- **Commands fix:** xash3d-fwgs provides `+alt1`/`+break`/`+commandmenu`/`ForceCloseCommandMenu`/`special`
  as ENGINE BUILT-INS (re-registering errors "already defined"). Trimmed `ts_commands.cpp` to register only
  the ones xash lacks (`+alt2`/`-alt2`, `buy`, `togglebrowser`); the built-ins forward to the server, which
  accepts them (cont.54). Clean run now: no "already defined", no crashes.
- **Verified:** builds (server+client), loads + runs on tm_avana, event hooked + precached, no errors.
  **NOT visually confirmed firing** (can't trigger a shot without interactive input) — the muzzleflash/shell/
  sound/anim need a user playtest shot; the path mirrors the working stock EV_FireGlock so confidence is high.
  AkimboFire still uses its own EMIT_SOUND (not moved to the event yet).

---

## 2026-06-22 (cont. 55) — Phase 2 (low-pitch fire sound): investigated, NO defect found in our code

Chased the cont.50 "regular fire sound is low-pitched/slow-mo" report. The fire-sound path is verifiably
CORRECT end-to-end — found no defect:
- Server emits ONE sound: `EMIT_SOUND(ENT, CHAN_WEAPON, wi->pszFireSound, VOL_NORM, ATTN_NORM)` =
  `EMIT_SOUND_DYN(..., 0, PITCH_NORM)` (ts_gun.cpp StandardFire/AkimboFire). Normal pitch.
- No duplicate source: the client's `EMIT_SOUND_DYN` is a no-op stub (cl_dll/hl/hl_baseentity.cpp:33),
  and the client only hooks the stock HL events (ev_hldm.cpp), none of which fire for `weapon_tsgun`
  (we don't PLAYBACK_EVENT stock events for it).
- No global time-scale: our slow-motion only sets the per-player member `m_flSlowMotionGo`; nothing
  touches host_framerate/maxspeed/etc., and sound pitch isn't pitched server-side by slow-mo at all.
- Files fine: TS fire wavs are 8-bit mono 22050 Hz — byte-IDENTICAL in format to the valve sounds xash
  plays at correct pitch. xash audio runs @44100 and resamples 22050 fine (valve sounds prove it).
- Engine note (benign): xash warns `CL_LoadProgs: failed to get address of HUD_GetSoundInterface` — the
  stock HL client doesn't export that xash3d-fwgs sound hook; absence → default sound (normal pitch).
**Conclusion:** no reproducible low-pitch in the current build (much changed since cont.50). Most likely
already-fixed, or was heard during an actual slow-motion state (correct behaviour). Did NOT edit the
correct sound code. If it recurs: capture which weapon, slow-mo or not, all-sounds-vs-fire-only; the
faithful long-term path is the predicted `events/fire.sc`→EV_TSFire (Phase 5), which plays at PITCH_NORM
too (won't change pitch). Cannot self-verify audio (no way to trigger a shot + capture without input).

---

## 2026-06-22 (cont. 54) — Phase 3: TS client cvars + commands registered (kills "Unknown command" spam)

Wired the TS-specific client cvars + console commands from the API surface scan, fixing the playtest
"Unknown command" spam + non-erroring keybinds.
- **Cvars (`cl_dll/ts_hud.cpp` TSHud_RegisterCvars):** registered ~25 TS client cvars with the EXACT
  defaults recovered from client.dll (API_SURFACE.md) — hud_r/g/b/color, cl_realhud/motionblur/gore/laser/
  lasercolor/solidcross/usepowerup/buykevlar/showequipment/shadow/thirdtransparency/camtype/attackswitch/
  thirdpersonswitch/wavedetail/fx/slowfade/vsmoothing, hud_classautokill/takesshots, nohud, r_paintball.
  (Registration only — most aren't *read* yet; stock/engine cvars left to the base client.)
- **Commands (`cl_dll/ts_commands.cpp`, `TSRegisterCommands` from InitInput):** `+alt1`/`-alt1`/`+alt2`/
  `-alt2`/`+break`/`-break`/`special`/`buy` forward to the server via ServerCmd; `+commandmenu`/`-commandmenu`/
  `ForceCloseCommandMenu`/`togglebrowser` are client UI stubs. Server (`dlls/client.cpp` ClientCommand) now
  accepts the forwarded TS inputs silently so they don't print "Unknown command" server-side.
- **Verified:** launched on tm_avana, NO "Unknown command" for any TS cvar/command in the log; game runs.
- **TODO (gameplay behind the commands, follow-on):** alt-fire/silencer-scope toggle (+alt1), coldcock
  (+alt2 = AlternateAttack@0x81ba8, found cont.53), stunts (special), the buy menu (cl_dll VGUI).

**Server cvar scan (task #5) DONE:** extracted the TS server cvars from the .so cvar_t structs (parsed
R_386_RELATIVE relocs → name/default pairs). Registered the ~25 TS-specific ones our server lacked in
`dlls/ts_cvars.cpp` (`TSRegisterCvars` from GameDLLInit) with exact defaults: mp_teammodels, initial_cash/
cashforwin/cashforlose, **roundtime 150 (WIRED → StartRoundTimer, replacing the hardcoded 120 — clock now
2:30)**, respawntime, damagemult, slowmatch, nopowerup, botlearn, realbullet, usecash, ammocount, bottalk,
mp_theonemode/theone/theothers, lastmanstanding, spectacularness, respawnnumber, customweapons, banfortk,
dmgreport, botarmor, deadstomach. Verified: no "Unknown command" for any TS cvar; round clock reads 150s.
TODO: server *console-command* + event enumeration (the standard mp_* alias names too); behaviour wiring
for the registered cvars (only roundtime read so far).

---

## 2026-06-22 (cont. 53) — DEEP RE of the HUD draw chain from client.dll (user mandate) → reference/HUD_DRAWCHAIN.md

User chose byte-exact RE over visual-match (visual can't verify alpha-blended fidelity). Traced the FULL
per-frame HUD render path from `client.dll` assembly using a new `scripts/ghidra/DecompAt.java` (decompile
a function by address; null-safe + whitespace-split args + dedup). Findings in **reference/HUD_DRAWCHAIN.md**:
- `HUD_Redraw`(export 0x1001aa90) → `CHud::Redraw` `FUN_10021aa0`(gHUD=0x100cf920) walks the element list,
  calls **`elem->vtable[+0xc](time)` = Draw**; `m_iFlags` at elem+0x10.
- The TS HUD is ONE singleton **`0x100d43cc`** (Init `0x10049b80` hooks all msgs; fields: health+0x20,
  armor+0x21, state+0x22, cash+0x24, flags+0x10, active-slot+0x744, weapon-id+0x7b8). Its **vtable =
  `0x1007cae4`**, Draw = vtable[+0xc] = **`0x1004d3f0`**.
- Draw `0x1004d3f0` dispatches: `FUN_1004e6f0` (static crosshair, 2×2 FillRGBA dots, cl_lasercolor),
  `FUN_1004e030` (**top weapon-select bar**: slot panels x=slot*0x80-0x78, y=8, w=0x80, h=0x14 + name text
  + SPR icon), `FUN_1004d700`/`FUN_1004d7e0` (**status+ammo box**), `FUN_1004e9f0` (dynamic spread
  crosshair), `FUN_1004d550` (bomb wires).
- **`FUN_1004d7e0` = the status/ammo box (the Five-seveN/MP5K panel):** each line = `FillRGBA(x=DAT_1018a67c,
  y=DAT_1018a700, w=0x80=128px, h≈lineheight, hud_r,g,b, **alpha 0x0f**)` + text `FUN_10021ce0(x+2,y,…)`.
  **Labels drawn at HALF brightness (color/2 = 75/87/127), values at full.** Numbers via `FUN_1004c290`.
- Engine fn globals: FillRGBA=`0x100d5b54`, SPR_Set=`0x100d5b38`, SPR_Draw=`0x100d5b44`.

**Applied (recovered, not guessed):** `TSHud_Panel` now fills with the HUD colour at **alpha 0x0f** (a
faint tint, matching FUN_1004d7e0) instead of the earlier guessed opaque dark navy. Built clean.

**THREE HUD regions (cont.53):** (1) **ammo box** ✅ exact geometry, verified; (2) **health BAR** ✅
`FUN_1004d5f0` — a fill bar (not text), 0x80 wide, fill ∝ health (red when low), bg alpha 0x1e, drawn
stacked above the ammo box; `TSHud_HealthBar` + shared `g_iTSHealth`, verified on-screen; (3) **weapon-
select bar** (top) ✅ DONE — server `CTSGun::SendWeaponStatus` sends `WStatus(3)={type,owned,clip}` per
arsenal weapon on change; client `CHudTSWeaponBar` (`cl_dll/ts_weaponbar.{cpp,h}`) draws held weapons in
5 category columns at the recovered `FUN_1004e030` geometry (x=col*0x80-0x78, y=8, 0x80×0x14), active
highlighted. Column per weapon = its `s_weapon_info+0x84` byte, EXTRACTED from the .so Write_<weapon>
funcs (the bar's grid TRANSPOSES the WStatus handler's (slot*0xb+pos): draw col = +0x84 [1=melee/grenade
2=pistols 3=SMGs 4=rifles/shotguns/heavy 5=akimbo], row = +0x85). Verified: "Beretta" shows in column 2.
**ALL THREE HUD REGIONS now covered** (weapon-bar + ammo box + health bar), each from recovered geometry.
**Text/font RESOLVED:** `FUN_10021ce0` draws per-char via `pfnDrawCharacter` advancing by SCREENINFO
charWidths, clipped to maxX — i.e. the ENGINE HUD font (no custom bitmap font; the "bitmapfonts" worry was
wrong). Ported faithfully as `TSHud_DrawString` (clip to panel width) + used by the ammo box & weapon bar.
**Finding (for the attachment system / +alt work):** `CTSGun::AlternateAttack @0x81ba8` = **ColdCock**
(pistol-whip melee bash) when the active clip > 0 — NOT the silencer toggle. So the attachment system
(silencer/laser support + toggle, which would light up the ammo box's Silencer/Lasersight rows +
`item_installed.spr` dots) lives elsewhere in the weapon code and is a distinct gameplay feature, not HUD
polish. The HUD attachment rows need the WeaponInfo flags byte (currently sent as 0) driven by that system.

**Weapon ICONS DONE:** the bar now draws each held weapon's `sprites/weapons/<name>.spr` (SPR_Load in
VidInit + SPR_DrawAdditive centred in the slot; name table from the client.dll string list, all 38 files
present in the mod). Verified: the Beretta pistol icon renders in column 2. Active weapon full-colour,
others dimmed. Remaining polish (low priority): the active-highlight timing + attachment dots
(`item_installed.spr`) which need the WeaponInfo flags byte. All in HUD_DRAWCHAIN.md.

**Anchor constants RESOLVED + APPLIED (cont.53, from the top of FUN_1004d7e0):** raw pixels off actual
screen size — **x = ScreenWidth−0x88(136), box w=0x80(128), lineH=0x14(20), lines stack UP from
y=ScreenHeight−8, strip = FillRGBA(...,alpha 0x0f)**, descriptive lines (caliber/mode) at half brightness,
name+ammo at full. Rewrote `cl_dll/ts_ammo.cpp` to this exact geometry → VERIFIED on-screen: "Beretta 92F /
9 x 19mm / Semi Auto / 15 / 45" right-anchored, dim labels, faint strips — matches reference Image #4/#6.

**STILL TODO to pixel-match (next, from HUD_DRAWCHAIN.md):** the TS **bitmap font** (`FUN_10021ce0`,
`bitmapfonts` cvar — we use DrawConsoleString); the **left status block** (health/armor/free-slots — trace
`FUN_1004d700`?) + **weapon-select bar** (`FUN_1004e030`, WStatus reader) with the same recovered approach;
the WeaponInfo **flags byte** server-side so the Silencer/Lasersight attachment rows draw. New Ghidra
tooling: `DecompAt.java`, `DecompRefs.java`.

---

## 2026-06-22 (cont. 52) — ammo HUD (WeaponInfo) + the COMPLETE client API surface scan + steel-blue panels

**Ammo HUD (`WeaponInfo`) reconstructed + VISUALLY VERIFIED.** Decoded the `s_weapon_status` slot struct
(0x38 bytes) from `CTSGun::TakeWeapon @0x80a04`: slot+0x4=clip, +0x2=fireMode, +0x8=caliber, +0x10=flags.
So `WeaponInfo`(6) = BYTE weaponType, BYTE clip, SHORT reserve, BYTE fireMode, BYTE flags — and
CTSGun NEVER sends `ClipInfo` (clip rides in WeaponInfo). Server: `CTSGun::UpdateClientData` override
(`dlls/ts_gun.cpp`) sends it on-change (type/clip/reserve[caliber]/mode; flags=0 SLICE). Client:
`CHudTSAmmo` (`cl_dll/ts_ammo.{cpp,h}`) draws the original's stacked box — name / caliber / fire-mode /
"clip / reserve". Screenshot on tm_avana: "Beretta 92F / 9 x 19mm / Semi Auto / 14 / 45". User sent
reference screenshots confirming the exact layout (Five-seveN/MP5K panels) + a left-HUD (team/health-
heart/free-slots) + the weapon-select bar.

**HUD colour + style recovered from client.dll (the user pushed for disasm of the draw calls):**
- `pfnFillRGBA` is engine-table offset 0x2c; the HUD panels are FillRGBA with VARIABLE (cvar-driven)
  colours, called via gEngfuncs @ `0x100d5c70`.
- **The HUD text colour = the `hud_r`/`hud_g`/`hud_b` cvars, RegisterVariable defaults 150/175/255**
  (steel blue) — `hud_color "150,175,255"`. Recovered by parsing the RegisterVariable call args.
- New `cl_dll/ts_hud.{cpp,h}`: registers hud_r/g/b/hud_color/cl_realhud (faithful defaults), `TSHud_Color`
  reads them, `TSHud_Panel` draws the dark translucent backing. ts_status + ts_ammo now use steel-blue
  text + panels. **NOTE (user-flagged): panel geometry/alpha are still APPROXIMATE — not yet traced from
  the original Draw chain (task #4).**

**FULL CLIENT API SURFACE SCAN → `reference/API_SURFACE.md` (new, tracked).** The user's directive: the
complete engine↔client contract = the behavioural surface; scan it all + ledger completeness. Recovered
ALL of it from client.dll by finding the engine func-ptr-global call sites (`ff 15 <ptr>`):
- `0x100d5b60` RegisterVariable → **104 cvars** (TS-specific ~30: hud_r/g/b ✅, cl_realhud/motionblur/gore/
  laser/solidcross/usepowerup/thirdperson/…; rest stock).
- `0x100d5b6c` AddCommand → **92 commands** (TS-specific: `+alt1`/`+alt2`/`+break`/`special`/`buy`/
  `togglebrowser`/`+commandmenu`/`ForceCloseCommandMenu`; rest stock +/- input). NONE implemented client-side.
- `0x100d5b70` HookUserMsg → **74 messages read** (the definitive read-side list; ✅5 readers, 🟡1, ❌~24 TS).
- `0x100d5c3c` PrecacheEvent → **11 events** (closecombat/coldcock/draw/empty/fire/firelinked/grenade/
  katana/knife/pump/reload) — all ❌ client EV_ callbacks (server playback ✅).

**Honest state correction (the user's point):** the SERVER was reconstructed surface-first via the ENTITY
surface (FGD + .so factories); the CLIENT API surface was being done reactively (per playtest bug), not
enumerated first. API_SURFACE.md now fixes that for the client. Still partial: server cvar/command/event
scan (task #5), and the EXACT HUD draw chain (geometry/alpha/font) from assembly (task #4 — the current
panels are approximations, which is why the look isn't pixel-matching the original yet).

**Built + ran** (amd64 server+client) in xash3d-fwgs; ammo box verified on-screen. cont.51's health/armor
panels also restyled. No message-size warnings.

---

## 2026-06-22 (cont. 51) — Phase 4: TS custom HUD begins (health/armor/cash) + the user-message wire contract recovered + 3 protocol-name bugs fixed

**Phase 4 (the TS custom HUD) started.** First recovered the FULL TS user-message contract from the
`.so`, then landed the first faithful HUD elements (health/armor/cash) end-to-end (server send + client
draw), VISUALLY VERIFIED in xash3d-fwgs.

**The message contract — recovered + documented in `reference/HUD_MESSAGES.md` (new, tracked).** Parsed
`LinkUserMessages @0xc7db0` (every REG_USER_MSG name + fixed size) and decompiled the two senders —
`CBasePlayer::UpdateClientData @0x81d00` and `CTSGun::UpdateClientData @0x758c8` — for the exact wire
packing. The TS block (names ON THE WIRE, sizes, format): `TSHealth`(1)=BYTE round(health)≥0,
`TSArmor`(1)=BYTE round(armor), `TSState`(1)=BYTE state[0x498], `TSCash`(2)=SHORT cash, `TSSpace`(1)=BYTE,
`WeaponInfo`(6)=BYTE wtype,BYTE,SHORT reserve,BYTE,BYTE (from the per-arsenal-slot struct @gun+0xcc, NOT
our s_weapon_info subset), `WStatus`(3)=per-slot BYTE×3, `ActItems`(2)=MSG_ALL BYTE entindex+BYTE,
`TSPAward`(2)=MSG_ALL, `PwUp`(3), `PwUpTime`(1), `RoundTime`(2)=BYTE secs+BYTE flag(0 start/2 timeout),
`TSMBlur`(2), `TSSlowMo`(2)=COORD(slowamt*100), `TSBTime`(1), `Wires`(3), `TSFrags`(-1), `TSPSys`(-1)…
Engine write-fn GOT offsets: `190fe0`=Byte `190fe8`=Short `190fec`=Long `190ff4`=Coord.

**3 inherited PROTOCOL-NAME/SIZE bugs fixed (swap-compat — the cont.50 mandate):**
- `RoundTime`: was registered -1 + written as TWO SHORTS (server) / read as two shorts (client). Original
  is **fixed size 2 = two BYTES** (seconds, flag). Fixed `dlls/ts_gamerules.cpp` (StartRoundTimer +
  SendRoundTime → WRITE_BYTE×2, REG size 2) AND `cl_dll/roundtime.cpp` (READ_BYTE×2). Clock still
  counts down correctly (verified "1:42"); 120s round max fits a byte.
- `TSMotionBlur` → **`TSMBlur`** (size 2) — wrong NAME + size (`dlls/ts_player.cpp`).
- `TSPowerUpTime` → **`PwUpTime`** (size 1) — wrong NAME (`dlls/ts_player.cpp`).

**Health/armor/cash HUD — DONE + VISUALLY VERIFIED:**
- Server (`dlls/ts_player.cpp` `CBasePlayer::TSUpdateClientData()`, called at the end of
  `CBasePlayer::UpdateClientData`): lazily registers + sends `TSHealth`(byte)/`TSArmor`(byte)/`TSCash`
  (short) on-change, cached in new `player.h` members (`m_iClientTSHealth/Armor/Cash`, reset on HUD init
  so they resend after respawn). Byte-faithful to the original (round+clamp health, round armor). The
  original sends THESE instead of stock Health/Battery — TS has a fully custom HUD.
- Client (`cl_dll/ts_status.{cpp,h}`, new — `CHudTSStatus`): hooks TSHealth/TSArmor/TSCash, draws
  "Health: N" / "Armor: N" / "$N" as text bottom-left (DrawHudString, gold / red when health≤25). Wired
  into hud.h/hud.cpp/CMakeLists like CHudRoundTime. Built both halves (amd64), nm/strings confirm the
  factories + hook names; ran in xash3d-fwgs on tm_avana → screenshot shows **"Health: 100" / "Armor: 0"**
  bottom-left + the round clock. No "Bad message" size warnings in the log.

**STILL TO DO in Phase 4 (next):**
- **AMMO HUD (the big one) — `WeaponInfo`/`WStatus` need the per-arsenal-slot struct (@gun+0xcc, 0x70/slot)
  decoded** to pin the 6 WeaponInfo bytes byte-faithfully (weapontype + reserve are known; the clip + the
  3 middle bytes come from the slot struct, which our modern `CTSGun` stores differently). CTSGun sends
  WeaponInfo/WStatus, NEVER ClipInfo — so the current clip rides inside WeaponInfo's slot snapshot. This
  is a consult-worthy ambiguity (raised to the user/prev-agent): exact slot-struct field meanings.
- `TSState` (player state byte @0x498 — semantics TBD), `TSSlowMo` (COORD slowamt — the visible
  time-dilation HUD), `TSBTime`, `PwUp`/motion-blur HUD readers, `TSFrags`/scoreboard.
- The leftover **stock HUD strip** still bleeds through at the very bottom (faint "00 |", player icon, "0")
  — the stock sprite HUD half-loading; should be suppressed once the TS HUD is complete.

**Verify loop used:** `scripts/build.sh amd64` + `amd64 client` → `setup-xash-mod.sh` → launched xash
(killed the 26-min idle prior instance first) → `import -window root` → cropped the 2560x1080+1280+208
window (`xwininfo` for geometry). Left a fresh instance running for the user.

---

## 2026-06-22 (cont. 50) — playtest reconstruction (reload, akimbo anims, HUD res) + the COMPAT TARGET clarified + docs sync

**Compatibility target clarified by the user (now in CLAUDE.md + HANDOFF):** the goal is INTERFACE/
protocol compatibility so each half is a **drop-in swap** for the original — our client ↔ the original
server (or vice-versa) behaves identically. Byte-faithful: the engine C interface + the client↔server
protocol (user-message **names + wire formats**, **event** names/params, **command** names, **cvar**
names). NOT a target: the internal C++ ABI / member offsets / gcc-2.x quirks (we build on the newer
Itanium-ABI halflife-updated SDK). This is exactly why `TSRoundTime`→`RoundTime` mattered, and it makes
protocol-level decomp fidelity mandatory for every message/event/command/cvar going forward.

**Done this session (all decomp-grounded, committed):**
- **Akimbo fire anims (e5ca177)** — `AkimboFire` SendWeaponAnim's the matching barrel's shoot sequence
  (right=`iFireAnim`, left=`iFireAnimL`, parsed from the `v_*_akimbo.mdl`). User-confirmed: anims + flare
  side switch correctly.
- **HUD sprite-resolution fallback (e5ca177, `cl_dll/hud.cpp`)** — TS's `hud.txt` is 320/640-only; the
  modern client picks `m_iRes`=1280 at 2560-wide → matches nothing → no sprite HUD. `CHud::VidInit` now
  falls back to the largest res present. NECESSARY BUT NOT SUFFICIENT: TS has a *fully custom* client HUD,
  so there is still NO HUD at all (no health/ammo/crosshair) — that's Phase 4 below.
- **Reload (241ba55, `dlls/ts_gun.cpp` + weapon table)** — root cause of "fire til empty, no reload": the
  stock `CBasePlayerWeapon::ItemPostFrame` gates reload on the player's standard `m_rgAmmo`, which TS never
  populates (ammo is in `CTSGun::m_ammoReserve` per caliber). Added `CTSGun::ItemPostFrame` that drives the
  TS ammo system: reload on `+reload` or auto when the clip runs dry, refill from the per-caliber reserve
  after `TS_RELOAD_TIME`, plays the per-weapon `reload` viewmodel sequence (`iReloadAnim`, parsed from each
  `v_*.mdl`). Built clean, runs in xash without crash (awaiting user playtest confirmation).

**User mandate: reconstruct the player/weapon/input/HUD systems WHOLESALE from the decomp** (stop the
symptom-patching). The 5-phase plan is tracked as tasks #6–#10 and in HANDOFF "Track B playtest
reconstruction": (1) weapon firing/reload/ammo/fire-rate [reload done], (2) CBasePlayer thinks/state + the
low-pitch fire sound (suspect slow-motion time-scale), (3) all TS commands/cvars/+toggles server+client,
(4) the TS client HUD (why there's no HUD), (5) client view bob/sway + predicted fire events.

**Open playtest reports (unfixed):** no HUD at all (Phase 4), regular fire sound is LOW-PITCHED ("slow-mo",
Phase 2 — confirmed pitch not the file: `beretta/fire.wav` is normal 22kHz), akimbo fire SOUND missing
(`g_weaponInfo[8].pszFireSound` is null — akimbo should fall back to the single's sound; quick Phase-1
fix), viewmodel bob "snap" (user confirmed `cl_bob 0` stops it; Phase 5), fire-rate feels ungated (Phase 1).

**Docs synced (this entry):** CLAUDE.md (compat-target rewrite + a Running/testing section: xash run loop,
`pgrep -x xash3d` not `pkill -f`, GPU-present), HANDOFF.md (compat target + the 5-phase plan + cont.47–50
status), RECONSTRUCTION.md (status rows). CLAUDE.md stays local-only (gitignored); the rest committed.

---

## 2026-06-21 (cont. 49) — TRACK B playtest pass: loadout, mouselook, fire animations (decomp-driven)

The user playtested the running client (cont.48) and reported HL leftovers. Fixed three, each
decomp-grounded (the user explicitly asked to check the decomp, not guess). The dev box launches the
game for the user via `setsid env LD_LIBRARY_PATH=. ./xash3d -game ts +map tm_avana &` (kill the old one
with `pgrep -x xash3d` — NOT `pkill -f client-x64/xash3d`, which matches the shell's own cmdline and
SIGTERMs your session). A detached/unfocused instance exits clean (~exit 0) after a bit; it stays up while
the user interacts.

1. **Spawn loadout (commit 54dac48, `dlls/ts_teams.cpp`).** Players spawned with HL crowbar+glock because
   `CTSGameRules::PlayerSpawn` chained `CHalfLifeMultiplay::PlayerSpawn`. Decompiled the real
   `CHalfLifeMultiplay::PlayerSpawn @0x790ac`: it honours `game_player_equip`, else
   `GiveNamedItem("weapon_tsgun")` (PIC strings resolved via GOT-0x6dda6) + `BuyArmament()` (cont.36).
   Reconstructed exactly → no HL weapons; spawn gives the unified TS gun (default `m_weaponType` =
   TSW_BERETTA, so you get a beretta) + the armament. (`CHalfLifeMultiplay::PlayerSpawn` itself is an
   11-byte forwarder to FUN_000890b8; `CBasePlayer::TSInit @0x92a4c` inits CTSAnimation/CTSStunt +
   `SetPlayerSpeed(330)`, gives no weapons.)
2. **Mouselook (commit + `cl_dll/inputw32.cpp`).** "Can't look up/down, only side-to-side; moving jolts
   the view fwd/back" = mouse-look OFF (mouse-Y walks you instead of pitching). The rig config never
   issues `+mlook`; TS is always-mouselook. Forced `in_mlook.state |= 1` in `IN_Init`. User CONFIRMED
   fixed. (Note: the *viewmodel* jolting back-and-forth is SEPARATE — still open; it's the client view
   bob/lag/sway, `cl_bob "3"` in the TS config + stock V_CalcBob/V_CalcViewModelLag — needs the client
   view reconstruction.)
3. **Fire animations (commit c607b75, `dlls/ts_weapons.{h,cpp}` + `ts_gun.cpp`).** Weapons fired
   (bullets/sound/muzzleflash already present) but the viewmodel never animated. Added `iFireAnim` to
   `s_weapon_info`, populated per-weapon from the actual "shoot*" sequence index parsed out of each
   `v_*.mdl` (studiohdr numseq/seqindex at offsets 164/168; 176-byte seqdesc; beretta shoot=1, glock=3,
   m4=3, aug=2, shotguns shootsemi=6, …; -1 for melee/grenade/akimbo). `StandardFire` now
   `SendWeaponAnim(wi->iFireAnim)`. SLICE: server-driven (visible, not predicted).

**Firing event system — fully MAPPED from the decomp (the faithful follow-up, task #6).** `CTSGun`
precaches 11 events (`events/fire.sc` → `m_usFire` @0x964, +coldcock@0x966/pump@0x96a/closecombat@0x96c/
firelinked@0x96e/reload@0x970/knife@0x972/katana@0x974/draw@0x976/grenade@0x978/empty@0x968). `StandardFire
@0x824a4` plays it: `PLAYBACK_EVENT(flags=1, player_pev, m_usFire, delay=0, vecZero, vecZero, fparam1=spread,
fparam2=spread, iparam1=m_weaponType(0xc8), iparam2=item+0x10, bparam1=(clip==0), bparam2=player[0x1fb])`.
The faithful client side = hook `events/fire.sc` → `EV_TSFire` reading iparam1=weapontype → predicted
sound + viewmodel anim + muzzleflash. NOT yet built (needs the client.dll EV callback). Our current sound
is a server EMIT_SOUND of `weapons/<w>/fire.wav` (22kHz, correct format); the user's "slow-mo sound"
report is unresolved (headless = no audio) — clarify low-pitch vs wrong-file vs droning.

**Still open (Track B client reconstruction):** the viewmodel jolt/bob, the predicted fire event (sound+
anim), the akimbo/melee/grenade fire anims, and the HL weapon-select HUD bar.

---

## 2026-06-21 (cont. 48) — TRACK B: the client RUNS + RENDERS in xash3d-fwgs; round-time HUD VISUALLY VERIFIED

**Headline: the dev box is NOT headless — it has a real GPU + X display (AMD RX 9070 XT, DISPLAY=:0).**
The cont.46 "headless, build-only, visual needs the user" caveat was WRONG. mod-ts client work can be
**self-verified with screenshots here** (`import -window root`). This changes the Track B workflow:
build → run in native xash3d-fwgs → screenshot → confirm. Use it.

**Done — mod-ts now runs end-to-end in native xash3d-fwgs (server + OUR client + content), and the
cont.47 round-time HUD is confirmed on-screen: a yellow `1:51` M:SS countdown at top-centre.**

1. **`scripts/setup-xash-mod.sh` (new) — the deliverable** ("configure xash to run this mod"). Assembles a
   native game root at `xash3d-fwgs/dist/client-x64` (which already holds the built engine: xash3d +
   libxash + libref_gl + libmenu + filesystem_stdio + libvgui). It builds `ts/` (content symlinks to
   `~/Containers/hlds/mods/ts`, our libs, a direct-gamedll liblist.gam — no metamod), completes the
   `valve/` fallback, and exposes the openvgui lib as `vgui.so`. **Three gotchas it encodes:**
   - xash on **x86-64 looks up the ARCH-SUFFIXED name** `hl_amd64.so` / `client_amd64.so` (the
     `COM_GenerateServerLibraryPath` `#else` branch — `XASH_X86` is 32-bit only). Misnamed `hl.so` was
     SILENTLY skipped → it loaded `valve/dlls/hl_amd64.so` instead (symptom: `No spawn function for
     "ts_hack"/"ts_sellzone"/...` for the LATER-added TS entities while older ones worked). Fix: name our
     libs `*_amd64.so` (bare name kept as an alias).
   - the cont.46 client links `DT_NEEDED vgui.so`; the engine ships the identical openvgui as
     `libvgui.so` (byte-for-byte = mod-ts/linux/release/vgui64.so) → symlink `vgui.so -> libvgui.so` and
     launch with `LD_LIBRARY_PATH=.`. libSDL2 resolves from the system.
   - Launch: `cd dist/client-x64 && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana`.
2. **Client crash fix (commit f484842) — `cl_dll/com_weapons.{cpp,h}` + `cl_dll/hl/hl_weapons.cpp`.**
   First actual RUN of the client SIGSEGV'd (user caught the "Xash Error: signal 11" dialog). gdb
   backtrace: `HUD_PostRunCmd → HUD_InitClientWeapons → HUD_PrepEntity → CGlock::Spawn → 0x0`. Pinned the
   exact insn (`call *g_engfuncs+536` right after loading the "weapon_9mmhandgun" string): on **64-bit**,
   `MAKE_STRING()` = `ALLOC_STRING()` = `(*g_engfuncs.pfnAllocString)` (util.h `#if __x86_64__`), but
   `HUD_InitClientWeapons` populates only a SUBSET of `g_engfuncs` and never sets `pfnAllocString` → the
   first `MAKE_STRING()` in any predicted weapon's Spawn deref'd NULL. Fix: a small client string pool +
   `stub_AllocString`/`stub_StringBase` (com_weapons), with `gpGlobals->pStringBase` pointed at the pool
   in HUD_InitClientWeapons so MAKE_STRING/STRING stay consistent (offset 0 reserved as ""). This is a
   generic halflife-updated 64-bit client-prediction bug, not TS-specific.
3. **Round-time-on-join (commit fd5146a) — `dlls/ts_gamerules.{h,cpp}` + `ts_teams.cpp`.** `StartRoundTimer`
   only broadcasts `RoundTime` at arm-time, so a client joining mid-round saw a blank HUD. Added
   `CTSGameRules::SendRoundTime(player)`, called from `InitHUD`, to catch a joiner up (no-op if unarmed).

**Verified (RUNTIME + VISUAL):** `setup-xash-mod.sh` → `LD_LIBRARY_PATH=. ./xash3d -game ts +map tm_avana`
→ strace confirms it loads OUR `ts/dlls/hl_amd64.so` + `ts/cl_dlls/client_amd64.so`; gdb-clean for a full
35s session (only SIGINT, no SIGSEGV); screenshot shows the world rendering (viewmodel, map) + the
**yellow `1:51` round clock** top-centre, window titled "mod-ts (faithful reconstruction)". The round
timer armed (counting from 120s) and the client read+drew the message. Server-side regression: tm_avana +
ts_dust2 load with 0 "No spawn function" once the dll name was fixed.

**Next Track B slices (now screenshot-verifiable):** the powerup/motion-blur indicators
(`TSPowerUpTime`/`TSMBlur` — verify names vs the .so first), the ammo/fire-mode HUDs, then the visible
slow-motion time-dilation + the particle system. Also noted but deferred: the stock client logs
`Unknown command cl_realhud / hud_r/g/b/color / mp_teammodels` — TS client cvars the original defined,
not yet reconstructed (harmless). The bg game process exits cleanly (exit 0) after a bit when launched
detached/unfocused — not a crash; interactive runs stay up.

---

## 2026-06-21 (cont. 47) — TRACK B: first TS client HUD — the round-time countdown (CHudRoundTime)

**Done — the first piece of TS client code: `cl_dll/roundtime.{cpp,h}` (CHudRoundTime).** The round-time
countdown HUD, reading the message the server already sends (cont.45). This is the first TS reader in
the cl_dll — proves the Track B loop (server sends a user message → client HUD element hooks + reads +
draws) end to end at build/load level.

**Fidelity fix to the server first (`dlls/ts_gamerules.cpp`):** the original `ts_i386.so` registers the
round-time user message as the string **`"RoundTime"`** (the C++ global is named `gmsgTSRoundTime` — both
appear in the unstripped symtab/.rodata; the registered STRING is "RoundTime", confirmed at .so .rodata
0x95900 where client.dll also hooks it). The inherited cont.45 server registered `"TSRoundTime"` — a
fidelity bug (client.dll hooks "RoundTime"). Changed `REG_USER_MSG("TSRoundTime")` → `REG_USER_MSG("RoundTime")`;
kept the var name `gmsgTSRoundTime` (faithful). Wire format unchanged: two shorts — `WRITE_SHORT(seconds
remaining, rounded)` + `WRITE_SHORT(0)` (the second is a flag word, 0 in the shipped binary; matches the
original `RestartRound` send @ the .so).

**Client element (mirrors the stock CHudBattery/Health pattern):**
- `CHudRoundTime : CHudBase` — `Init` hooks the `RoundTime` message (`HOOK_MESSAGE(RoundTime)`) + adds
  itself to the HUD list; `MsgFunc_RoundTime` reads the two shorts, turns `seconds` into an absolute
  client-time expiry (`m_flEndTime = gHUD.m_flTime + seconds`) so the clock keeps ticking locally between
  server updates (seconds<=0 clears it + drops HUD_ACTIVE); `Draw` renders `M:SS` (sprintf `%d:%02d`)
  centred just below the top of the screen via `DrawHudString` + `GetConsoleStringSize`, yellowish
  (RGB_YELLOWISH), flashing red (RGB_REDISH) in the last 10s; `Reset` clears it.
- Wired in: `cl_dll/hud.h` (`#include "roundtime.h"` + `CHudRoundTime m_RoundTime;`), `cl_dll/hud.cpp`
  (`m_RoundTime.Init()` + `.VidInit()`), `cl_dll/CMakeLists.txt` (`roundtime.cpp` in the source list).

**Verified (BUILD + LOAD-level, per the cont.46 caveat — this headless box can't RENDER the client):**
- `build.sh amd64 client` → 64-bit client.so (openvgui), glibc floor 2.14; `build.sh i386 client` →
  32-bit client.so (Valve vgui), glibc floor 2.3 — both clean. `nm | c++filt` shows the linked
  `CHudRoundTime::{Init,VidInit,Draw,Reset,MsgFunc_RoundTime}`; `strings client.so` shows the `RoundTime`
  hook string.
- `build.sh amd64` (server) → hl.so clean; `strings hl.so` shows `RoundTime` (not TSRoundTime).
- Server no-regression: booted `tm_avana` on the rig (our hl.so) → GAME SKILL LEVEL reached, 0 Can't init,
  no crash over 20s. (The message rename is server-string-only; doesn't touch the verified cont.45 timer
  logic.)

**VISUAL CONFIRMATION STILL OWED (coordinate with the user):** the actual on-screen M:SS clock needs a
graphical GoldSrc/xash client run by the user — build/load here can't show pixels. The likely visual
tweaks once seen: exact screen position/size, whether TS draws it as a clock sprite vs. text, and the
flash threshold. Treat position/colour as // VERIFY.

**Next Track B slices** (all read messages the server already sends): the powerup/motion-blur indicators
(`gmsgTSPowerUpTime` / `gmsgTSMotionBlur` — the .so registers `TSMBlur`, `TSSlowMo`, `TSState`, `TSHealth`,
`TSArmor`, `TSCash`, `TSFrags`, `TSBTime`, `TSPSys` — CHECK each inherited gmsg name against the .so
strings the same way RoundTime was fixed), then the bigger visible-slow-motion time-dilation (client
pmove scales frametime by the slow fields) and the particle system (CTSSharedEmitter/CTSEmitter/CTSParticle
via the `TSPSys` message).

---

## 2026-06-21 (cont. 46) — TRACK B START: the client (cl_dll) now builds, both 32-bit + 64-bit

**Done — the cl_dll build foundation.** The mod-ts client library (`cl_dll/` → `client.so`) now
builds in the buildchain for BOTH i386 and amd64. This is the prerequisite for ALL Track B (TS client
HUDs, visible slow-motion, particles, stunts) — none of which could start before the client built.

**Key finding (answers "how did halflife-updated link vgui for 64-bit?"):** it never did. `build.sh`
only ever built `--target hl` (the server); NO `client.so` exists anywhere in halflife-updated's build
tree. The bundled `vgui.so`/`libSDL2` were always 32-bit; the 32-bit-VGUI wall was simply never hit
because nobody built the client target in the 64-bit buildchain. Track B is the first time.

Three portability fixes (committed 8f3b248) to compile the stock client under the buildchain's gcc-8.3:
- `cl_dll/particleman/CMiniMem.h`: replaced `std::pmr::unsynchronized_pool_resource` (<memory_resource>,
  gcc-9+) with a minimal `operator new`-backed pool shim (same allocate/deallocate/release surface).
- `cl_dll/particleman/CFrustum.cpp`: dropped `<GL/gl.h>` (absent in buildchain) — it only needs the
  GL_MODELVIEW_MATRIX/GL_PROJECTION_MATRIX enum values for the engine TriAPI; defined locally.
- `cl_dll/entity.cpp`: `(int)pTemp` (FTENT_SPIRAL phase) is a 64-bit-pointer→int truncation → route
  through intptr_t.

Dual-arch VGUI (this commit), per the user's pointer to xash3d-fwgs:
- **32-bit** (i386) links Valve's bundled `vgui.so` (Steam HL); **64-bit** (amd64) links openvgui's
  ABI-compatible `libvgui.so` — vendored as `linux/release/vgui64.so` (from xash3d-fwgs, x86-64), plus
  a 64-bit `libSDL2` vendored as `linux/libSDL2-2.0-x64.so.0`. openvgui is a drop-in: the client
  compiles against the STOCK `../utils/vgui/include` headers and links clean against it (no source
  changes — just a relink). Both are only load-time linked (the engine supplies the real libs).
- `cl_dll/CMakeLists.txt`: selects the arch-appropriate vgui/SDL2 by `CMAKE_SIZEOF_VOID_P`; build-dir
  filenames stay `vgui.so`/`libSDL2-2.0.so.0` so DT_NEEDED is uniform (the runtime provides the
  arch-correct lib under that name). `scripts/build.sh` gained a target arg: `build.sh <arch> [hl|
  client|all]` (default hl, backward-compatible). `linux/.gitignore` allows the two new vendored libs.

**Verified (BUILD-ONLY):** `build.sh amd64 client` → 64-bit client.so (openvgui), glibc floor 2.14;
`build.sh i386 client` → 32-bit client.so (Valve vgui), glibc floor 2.3; `build.sh amd64` still builds
the server (backward compat); `build.sh amd64 all` builds both. DT_NEEDED = vgui.so + libSDL2-2.0.so.0
for both arches.

**VERIFICATION GAP (important for Track B):** this headless box can BUILD client.so but cannot RENDER
it — the dedicated-server rig has no graphics, and the browser-playable client is the separate WASM
xash build, not this native client.so. So TS client work is build-verified here; visual/runtime
behaviour needs a graphical GoldSrc/xash client run by the user. Next Track B slice: a first TS HUD
(e.g. the round-time countdown reading gmsgTSRoundTime) — build + load verifiable, visual by the user.

---

## 2026-06-21 (cont. 45) — Round timer (CTSGameRules::RoundTimeLeft + StartRoundTimer)

**Done — the team-round time limit.** Completes the round lifecycle: team rounds now end three ways —
elimination (cont.37), objective (cont.25/31), and now TIME-OUT. Part of Track A's round polish.

RE (recovered/server/CTSGameRules.c): RestartRound @0x77920 sets the round end time +8 =
clamp(`roundtime` cvar, 30, 180) + time, clears +0xc..0xf, and (if the round-time flag +0x10)
sends gmsgTSRoundTime (remaining seconds). RoundTimeLeft @0x79024: when +8 elapses, fire the objective
manager's `targetontimeleft` (+0x554 → +0x94) then `TeamWin(2)` = a DRAW. Functional reconstruction:
- `CTSGameRules::StartRoundTimer()`: m_flRoundEnd = time + clamp(TS_ROUNDTIME=120, 30, 180) + register
  & send gmsgTSRoundTime (remaining seconds) to all (lazy REG_USER_MSG "TSRoundTime").
- `CTSGameRules::RoundTimeLeft()`: `TSFireTimeLeftTarget()` (new free helper in ts_objectives.cpp —
  fires the objective manager's m_targetTimeLeft) + `TeamWin(2)` (draw → schedules the round restart).
- `Think`: in team mode with no win pending, arm the timer on the first frame (m_flRoundEnd==0 →
  StartRoundTimer) and run RoundTimeLeft when it expires. `TSRestartRound` zeroes m_flRoundEnd so the
  next round re-arms.

**Verified RUNTIME (rcbot, tm_avana, temp probe reverted — shortened the timer to 4s):** full cycle
armed(4s) → expired → RoundTimeLeft draw (score [0,0] UNCHANGED = correct, a draw doesn't score) →
restart scheduled (+3s) → respawn → timer re-armed → … cycling cleanly. No crash; clean regression
(tm_avana + ctb_deadheat 0 Can't init, ts_wyndham only ts_emitter/ts_particle). (Ran on -port 27018.)

**SLICE:** TS_ROUNDTIME is a fixed 120s (the .so reads a `roundtime` cvar — VERIFY/wire it); the
round-time-enabled flag is implied by team mode (no per-mode gate); gmsgTSRoundTime is sent but no
cl_dll reads it (no round-time HUD). This leaves UpdateTeamScores/InitTeamScore + the full faithful
RestartRound (game-mode member + per-player round setup) + ShowObjectiveOnHud as the last Track A
polish; the big frontier is Track B (cl_dll).

---

## 2026-06-21 (cont. 44) — CTSPlayerAmount + CTSSuperJump (small self-contained leftovers)

**Done — `dlls/ts_playeramount.cpp` (CTSPlayerAmount) + `CTSSuperJump` in `dlls/ts_powerup.cpp`.**
Two of the small non-FGD-table leftovers; both self-contained, synthetic-verified.

RE: `CTSPlayerAmount` (Spawn @0x72a78, ClientConnect @0x72aec / ClientDisconnect) is a point relay
(ts_playeramount) — a per-entity counter (+0x6c) that fires `connecttarget` (+0x74) when the count
rises to `clients` (+0x70) and `disconnecttarget` (+0xd8) when it falls back to it, driven off the
engine connect/disconnect events. `CTSSuperJump` (Spawn @0xc9f44) is `CTSSuperJump : CTSPowerUp` —
sets m_iType = 0x100 (TS_PWUP_LOWGRAV) then chains CTSPowerUp::Spawn, so it's a map powerup that
always gives the super-jump (low-gravity) effect. Reconstruction:
- `CTSPlayerAmount : CBaseEntity` — KeyValue clients/connecttarget/disconnecttarget; a 0.5s CountThink
  polls the live connected-player count and fires connecttarget on the rising edge to `clients` /
  disconnecttarget on the falling edge (a poll equivalent of the .so's connect/disconnect events, so
  no engine hook needed).
- `CTSSuperJump : CTSPowerUp` — Spawn forces m_iType = TS_PWUP_LOWGRAV then CTSPowerUp::Spawn.

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — temp probe reverted):** a ts_playeramount(clients=2)
fired connecttarget the moment the live count rose to 2 (rising-edge); ts_superjump spawned as a
powerup without crashing. Clean regression (tm_avana 0 Can't init; ts_theatre's 328 powerups still 0
Can't init — CTSSuperJump didn't disturb the powerup path). (Ran on -port 27018; 27015 held by an
unrelated app.)

**SLICE:** CTSPlayerAmount uses a poll instead of the engine connect/disconnect hooks + ignores its
spawnflags; CTSSuperJump inherits CTSPowerUp's Save/Restore as-is. Remaining leftovers: CInfoBreakable
(a respawning breakable — thin, deferred), CBulletManager (the traveling-bullet/bullet-time system —
deferred: it rewrites the verified instant-hitscan fire path + isn't visible without a cl_dll).

---

## 2026-06-21 (cont. 43) — CTSScoreGiver + CVipEscape (the last team-objective leftovers)

**Done — `CTSScoreGiver` + `CVipEscape` in `dlls/ts_objectives.cpp`.** The two small remaining
team-objective entities; closes out the Track A entity list. Both FGD-listed, 0 stock placements →
synthetic-verified.

RE: `CTSScoreGiver` (Spawn @0x8c0b8, Use @0x8c158) is `@PointClass base(WinGiver)` — identical to
CTSWinGiver (cont.25) but its Use RESETS the call counter after firing (param+0x19 = 0) so it scores
REPEATEDLY (vs WinGiver latching done), and a "Respawn Players" spawnflag. `CVipEscape` (Spawn
@0x8d23c) is classname **`func_vip_safetyzone`** (not ts_vipescape) — a passive brush marker (KeyValue
stub, empty Precache, non-solid SET_MODEL) the unbuilt VIP-escort logic checks against; it has no
touch/use of its own. Reconstruction:
- `CTSScoreGiver : CTSWinGiver` — Use counts to callsnumber, then bumps `CTSGameRules::m_teamScore`
  + HUD message + re-arms (m_calls=0). Scores DIRECTLY (not via TeamWin) so it's genuinely repeatable
  and doesn't restart the round (a score giver gives points without ending the round, unlike the win
  giver). // SLICE: the .so routes through TeamWin w/ cash; "Respawn Players" spawnflag not applied.
- `CVipEscape : CBaseEntity` (func_vip_safetyzone) — a non-solid brush marker. // SLICE: the VIP win
  detection is in the unbuilt team subsystem.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** ts_scoregiver callsnumber=2:
team0 score 0 → 0 (1 call) → 1 (2 calls = scored) → 2 (4 calls = re-armed + scored again, REPEATABLE,
no round-restart block); func_vip_safetyzone created without crash. Clean regression (tm_avana +
ctb_deadheat + ts_dust2 0 Can't init). (Ran on -port 27018; 27015 held by an unrelated app.)

**TRACK A ENTITY LIST COMPLETE.** All FGD team/CTF/objective entities are now reconstructed: teams
(cont.35) + cash/buy (36) + elimination (37) + CTSFlag (38) + CTSBriefcase (39) + CTSBomb (40) +
CTSSellZone (41) + CTSTeamEscape (42) + CTSScoreGiver/CVipEscape (43). Remaining Track A polish is
faithful niceties (UpdateTeamScores/InitTeamScore, the round timer, ShowObjectiveOnHud, CheckDeads
grace/CTB/last-man, capture-via-TeamWin-restarts-round). The big remaining frontier is **Track B —
the cl_dll** (visible slow-motion, the particle system CTSSharedEmitter/CTSEmitter/CTSParticle,
CTSStunt, all HUDs) — NONE built yet. Also the small non-FGD leftovers: CTSPlayerAmount,
CTSSuperJump, CInfoBreakable, CBulletManager.

---

## 2026-06-21 (cont. 42) — CTSTeamEscape (team escort/escape zone)

**Done — `dlls/ts_escape.cpp` (ts_teamescape).** A team escort objective: when enough of the
attacker team is simultaneously inside the zone, it completes (fire target + HUD message). FGD-backed
(`@SolidClass`, brush, base tsobjective); 0 stock placements → synthetic-verified.

RE (recovered/escape/CTSTeamEscape.c): Spawn @0x8d390 (brush SET_MODEL + SetSize), EnableObjective
@0x8d4a0 (enable + clear the 33-slot presence array + count), WaitTouch @0x8d4ec (attacker-team
player → track by entindex, count distinct; threshold from escapepercentage @+0xf4 over the attacker
TEAM SIZE: 0→1, 1→size/3, 2→2*size/3, 3→size; complete → FireTargets+HUD+EnableObjective(false)),
WaitThink @0x8d6b0 (clear the presence array + count every 1s — so it counts SIMULTANEOUS presence).
Functional reconstruction onto the stock SDK (inlined attacker-team gate like ts_objectives.cpp; team
size = live attacker-team players):
- `CTSTeamEscape : CBaseEntity` (brush SOLID_TRIGGER). m_present[33] (EHANDLE by entindex) + m_count.
- WaitTouch accumulates distinct attacker-team touchers; threshold per escapepercentage switch;
  completion fires target + message + disables. WaitThink resets the window each second.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** team size 2 — escapepercentage
0 (one member): one touch → completed (enabled=0); escapepercentage 3 (all): one touch stays active
(enabled=1), all touches → completed (enabled=0). No crash; clean regression (tm_avana + ts_dust2 0
Can't init). (Ran on -port 27018; 27015 still held by an unrelated app.)

**SLICE:** enable is faithfully gamerules-driven (here Spawn); no Save/Restore (the .so has
m_SaveData); completion fires the target only (no team score / round-win — that's the objective
manager's job); team size = live players (the .so reads the team struct via TSCountPlayers).

---

## 2026-06-21 (cont. 41) — CTSSellZone (sell dropped weapons for cash) + a powerup-precache fix

**Done — `dlls/ts_sellzone.cpp` (ts_sellzone).** The one entity that makes the cont.36 cash actually
flow: a brush zone that sells dropped weapons. FGD-backed (`@SolidClass`, base tsobjective; ts_dust2
places one).

RE (recovered/sellzone/CTSSellZone.c): KeyValue @0x726c0 (attackerteam), Spawn @0x72720 (brush
SET_MODEL + SetSize + SetTouch), SellTouch @0x727e8 — a ~61-byte NEAR-STUB that only does
`strcmp(toucher->classname, "WorldGun")`. Consistent with cont.36 (CashBased OFF → the sell economy
is vestigial in the shipped binary). Functional reconstruction:
- `CTSSellZone : CBaseEntity` (brush SOLID_TRIGGER). SellTouch: a touching `ts_worldgun` (a dropped
  weapon, cont.27) → credit the nearest living player (on the zone's team, if set) a flat
  SELLZONE_VALUE ($500) via `CBasePlayer::GiveCash` + a center-print, then UTIL_Remove the gun.

**Also fixed a pre-existing latent crash (surfaced by ts_dust2):** the dynamic DM powerup spawner
(cont.26 PutPowerUp → CTSPowerUp::Spawn) PRECACHE_MODELs `models/powerup.mdl` at RUNTIME — fine on
maps with ts_powerup placements (precached at load), but Host_Errors on a DM map with none (ts_dust2:
no ts_powerup, no objective_manager → DM → spawner runs → crash). Added `models/powerup.mdl` to
PrecacheAllTSWeapons (map load), same fix pattern as cont.38/40.

**Verified RUNTIME (rcbot, ts_dust2 — a REAL map with a sell zone, temp probe reverted):** the zone
spawns (0 Can't init, was 1 + the powerup crash gone); a dropped beretta worldgun dropped at a player
→ SellTouch credited the player cash 0→500 + marked the gun removed (FL_KILLME). No crash; clean
regression (ts_dust2 + tm_avana + ctb_deadheat all 0 Can't init).

**SLICE:** flat sale value (the per-weapon price s_weapon_info+0xd8 isn't extracted + is dead while
CashBased off); the dropped gun records no seller (no pev->owner) so the nearest player is credited;
no team-score / round effect.

---

## 2026-06-21 (cont. 40) — CTSBomb: the defuse / wire-cut objective

**Done — `dlls/ts_bomb.cpp` (ts_bomb).** A self-contained team objective: a planted bomb that counts
down and explodes unless defused. FGD-backed (`@SolidClass`, base tsobjective). 0 stock placements →
synthetic-verified (like ts_dmhill cont.32 / ts_trigger cont.34).

RE (recovered/bomb/CTSBomb.c): Spawn @0x8c470, Precache @0x8c570 (misc/beep2.wav), SetRandomWires
@0x8c598 (3 wires: one DEFUSE(1), one BOOBYTRAP(2), rest NEUTRAL(3)), EnableObjective @0x8c608 (arm/
safe), CutWire @0x8c70c (1→defuse+fire target/msg, 2→fast countdown, 3→timer to 0 = detonate),
WaitThink @0x8c860 (−1/tick, 1s or 0.1s when fast, beep rising pitch, detonate at 0 = TSExplosion),
Use @0x8c9a4 (hold USE `defusetime`s → defuse, FCAP_CONTINUOUS_USE=0x10), KeyValue defusetime/cutwires.
Functional reconstruction onto the stock SDK:
- `CTSBomb : CBaseEntity` (brush, inlined attackerteam gate like ts_objectives.cpp). Arms on Spawn
  (SLICE: faithful enable is gamerules InitObjectives). EnableObjective(true)=arm+SetRandomWires+
  WaitThink; (false)=safe.
- WaitThink: countdown −1/tick (1s, or 0.1s after a boobytrap), beep, `Explode` at 0 via
  `ExplosionCreate` (the SDK env_explosion blast — assets already precached by the weapon/grenade path).
- CutWire(0..2): defuse → FireTargets(target)+HUD msg+safe; boobytrap → m_bFast; neutral → timer≈0
  (next WaitThink detonates). `tscutwire <n>` console command (cl_dll wire UI is a SLICE).
- Use: continuous-USE accumulator over defusetime → defuse (FireTargets+msg).
- misc/beep2.wav added to PrecacheAllTSWeapons (map load) — a runtime-armed bomb's PRECACHE_SOUND
  Host_Errors (gdb/log-caught, same class as cont.38); dropped the Spawn-time Precache() call.

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** armed bomb wires=[2 1 3]
(one each); cut DEFUSE wire → enabled=0 (defused); cut NEUTRAL wire → timer 0.01 → WaitThink Explode
(ExplosionCreate runs at runtime, enabled=0, no crash); cut BOOBYTRAP wire → fast=1 still armed. No
crash; clean regression (tm_avana + ctb_deadheat 0 Can't init, ts_wyndham only ts_emitter/ts_particle).
(Test ran on -port 27018; 27015 was held by an unrelated app.)

**SLICE:** bomb countdown is a fixed 45s (the .so reads a bomb-time global DAT_001781a8 — VERIFY);
faithful arm is gamerules-driven (here Spawn); wire-cut needs the cl_dll UI (console-driven); no
Save/Restore (the .so has CTSBomb m_SaveData); detonation doesn't end the round / score (just blasts).

---

## 2026-06-21 (cont. 39) — CTSBriefcase: the CTF base/capture zone (seeds CTSFlag on a real map)

**Done — `dlls/ts_briefcase.cpp` (ts_briefcase).** The map-placed CTF objective that pairs with the
cont.38 CTSFlag: each team's briefcase seeds its flag at base and is the capture zone. Replaces
cont.38's `tsspawnflag` console stand-in — **first CTF entity verified on a real stock map**
(ctb_deadheat). FGD-backed `@SolidClass` (brush), ownerteam 1..4.

RE (recovered/briefcase/CTSBriefcase.c): ctor @0x8d788 (per-briefcase flag id s_nextFlagID 1..15,
team @+0x68 = -1, s_flag_status @+0x90), KeyValue @0x8d7f4 (contents/message/ownerteam), Spawn
@0x8d88c (brush SET_MODEL + SetSize), FlagFall @0x8da00 (CreateFlag(0, &s_flag_status) at the brush
centre + gmsgTSHaveFlag HUD + WaitThink), CanInteract @0x8db48 (team @+0x68 == player team),
CaptureTouch @0x8db9c (a living HaveFlag player who CanInteract → capture the carried flag).
Functional reconstruction onto the cont.38 CTSFlag:
- `CTSBriefcase : CBaseEntity` (brush SOLID_TRIGGER). KeyValue ownerteam is **1-based** in the FGD →
  stored 0-based (`m_iOwnerTeam = atoi-1`) to match pev->team / the objective gating; + contents +
  message (capture announce).
- Spawn: brush model + SOLID_TRIGGER; SetTouch(CaptureTouch); SetThink(FlagFall) @+0.5s.
- FlagFall: `TSCreateFlag(m_iOwnerTeam, Center())` — seeds this team's carriable flag at base (delayed
  so the runtime SET_MODEL follows the precache pass), then idle WaitThink.
- CaptureTouch: a living player carrying a flag (`TSPlayerHasFlag`) at THEIR own briefcase
  (`CanInteract`) → `TSReturnCarriedFlag` (return the carried flag) + `TeamWin(team)` + announce
  pev->message. (New shared helpers `TSPlayerHasFlag`/`TSReturnCarriedFlag` added to ts_flag.cpp.)

**Verified RUNTIME (rcbot, ctb_deadheat — a REAL stock CTB map, temp probe reverted):** 2 briefcases
spawn (0 Can't init, was 2), each FlagFall-seeds its flag at base with the correct 0-based team —
team 0 (Specialists) @(128 2048 192) + team 1 (Mercenaries) @(-128 -2048 192) (FGD ownerteam 1/2 →
0/1). Capture reuses the cont.38-verified TSReturnCarriedFlag + TeamWin. No crash; clean regression
(ctb_deadheat + tm_avana 0 Can't init, ts_wyndham only the known ts_emitter/ts_particle).

**SLICE:** the per-flag network id + gmsgTSHaveFlag HUD is server-only (no cl_dll flag indicator);
capture scores via TeamWin (which restarts the round — faithfully a capture awards cash + a HUD
without ending the round); no s_flag_status / contents drop; Save/Restore not wired (FCAP_DONT_SAVE).

---

## 2026-06-21 (cont. 38) — CTSFlag: the CTF flag (carry / capture / drop), unblocked by teams

**Done — `dlls/ts_flag.cpp` (ts_flag).** The first CTF entity, now possible because team
assignment (cont.35) gives a real pev->team. The classic capture-the-flag loop, gated on team.

RE (recovered/flag/CTSFlag.c): `CTSFlag : CTSPhysicObject`, model `briefcase.mdl`, runtime-created
(CreateFlag @0x9d874 — NOT map-placed: no FGD entity, 0 BSP placements; the CTSBriefcase map
objective seeds the bases). m_iFlagTeam @+0x9c, carrier EHANDLE @+0xa4, s_flag_status (7 ints @+0x90)
mirrored onto the carrier (player+0x7d0) via CBasePlayer::TakeFlag @0x93830 / DropFlag @0x9334c +
GetSpeedBySlots (carrying slows you). CanInteract @0x9e2d4 = flag.team==-1 || ==player.team.
PlayerTakeFlag @0x9e330 (Use/WaitTouch): living player; own-team flag → capture/return branch, else
TakeFlag (grab). FUNCTIONAL reconstruction onto the stock SDK (no CTSPhysicObject, no speed-by-slots):
- `CTSFlag : CBaseEntity` with a team + a 3-state machine (AT_BASE / CARRIED / DROPPED).
- FlagTouch: an enemy/neutral flag + a non-carrying living player → AttachTo (carry: SOLID_NOT,
  CarryThink rides the carrier's back, follows + drops if the carrier dies/leaves). Own-team flag at
  base + you're carrying the enemy flag → CAPTURE (return the carried flag + score via TeamWin).
- DropAt (the PlayerKilled `TSDropFlag` hook + CarryThink-on-death): flag falls where the carrier
  died, SOLID_TRIGGER, ReturnThink auto-returns it to base after 30s. ReturnToBase resets it home.
- `CTSFlag::Create(team, base)` + free `TSCreateFlag`/`TSDropFlag`; a `tsspawnflag <team>` console
  command seeds bases (stand-in for the CTSBriefcase map objective). briefcase.mdl + the pickup/drop
  sounds added to PrecacheAllTSWeapons (map-load) — a runtime PRECACHE_MODEL Host_Errors (gdb-caught:
  PF_precache_model_I → Host_Error → ShutdownServer; same class of bug as cont.12).

**Verified RUNTIME (rcbot, tm_avana, SYNTHETIC — temp probe reverted):** seeded a spec(0) + merc(1)
flag at a merc bot; merc touches the spec flag → carried=1 state=CARRIED; merc (carrying) touches its
own merc flag at base → CAPTURE: spec flag returned to base (state=AT_BASE), team-1 score 0→1; re-grab
→ TSDropFlag → state=DROPPED, carrier cleared. No crash; clean regression (tm_avana 0 Can't init,
ts_wyndham only the known ts_emitter/ts_particle).

**SLICE:** carrying doesn't slow the carrier (no GetSpeedBySlots / s_flag_status); no CTSPhysicObject
bounce physics; capture scores via TeamWin's team score (which also schedules a round restart — the
faithful capture awards cash + a HUD pointer without ending the round); flags are console-seeded (the
map-placed CTSBriefcase that creates them isn't built); no flag HUD/carrier icon (cl_dll).

---

## 2026-06-21 (cont. 37) — CheckDeads: team-round elimination win condition

**Done — `CTSGameRules::CheckDeads`/`CallCheckDeads` (ts_gamerules.{h,cpp}).** The automatic team-
round win condition: a round ends + the surviving team wins when the other team is wiped out. Builds
on cont.31's TeamWin/round-restart + cont.35's team assignment — team mode now RESOLVES rounds on its
own (independent of the objective entities, which remain an alternate win path; both call TeamWin).

RE (recovered/server/CTSGameRules.c): CheckDeads @0x79c00 (1335 bytes), CallCheckDeads @0x79bc0.
CallCheckDeads: if no restart pending (+0xc==0) schedule CheckDeads at time+1 (+0x5880). CheckDeads
(team branch, game-mode 2/3): count alive players per team into local_3c[4] via GetTeamIndex(TeamID());
mode-3 (CTB) fires objective targets; else winner = the team with survivors — local_3c[1]<1 → team 0
wins, local_3c[0]>0 → return (contested), else team 1 wins; nobody alive → team 2 (draw); then
TeamWin(winner,0,1). Functional reconstruction:
- `CallCheckDeads()`: if `m_flRoundRestart==0 && m_flCheckDeads==0` → `m_flCheckDeads = time+1`.
- `CheckDeads()`: team mode + no restart pending; count alive/present per team (general over
  m_numTeams); require ≥2 players over ≥2 teams (so warmup/one-sided doesn't insta-win); `teamsAlive>=2`
  → return (contested); `==1` → TeamWin(lastAliveTeam); `0` → TeamWin(2)=draw.
- `Think`: fire CheckDeads when `m_flCheckDeads` elapses (before the restart check).
- `PlayerKilled`: `CallCheckDeads()` after the death loot.
- `TeamWin` refined: guard against a double-win in a round (`m_flRoundRestart!=0` → return); team 0/1
  bump m_teamScore + announce `GetIndexedTeamName`, anything else = a draw announce (no score).

**Verified RUNTIME (rcbot, tm_avana, temp probes reverted):** balanced 1v1 → force-killed the
Mercenary → 1s later CheckDeads (present[1,1] alive[1,0] teamsAlive=1) → TeamWin(0): Specialists
score [1,0], restart scheduled +3s → round restarted (respawn all + objectives re-armed) → next round
2v2 = contested (no premature win). No crash; clean regression (tm_avana 0 Can't init, ts_wyndham only
the known ts_emitter/ts_particle).

**SLICE:** no just-respawned grace window (the .so counts +0x770>=0 as still-alive; we use IsAlive());
mode-3 (CTB) objective-target firing not reconstructed (treated as plain elimination); no round timer
(gmsgTSRoundTime) or last-man-standing DM mode (g_lastManStanding branch); TeamWin still lacks the
faithful UpdateTeamScores/InitTeamScore + the full RestartRound@0x67920 (round timer + game-mode member).

---

## 2026-06-21 (cont. 36) — Cash + buy/armament system (free loadout; CashBased is OFF)

**Done — `dlls/ts_buy.cpp`: the TS cash + buy/armament loadout (Track A step 2).** Reuses the
built `GiveTSWeapon`/`TSAddAmmo` arsenal.

**KEY RE FINDING:** `CashBased__Fv` @0x66abc is hardcoded `return 0` in the shipped .so — the cash
economy is COMPILED OUT. Every `if (CashBased())` cost-deduction branch in BuyWeapon/BuyAmmo/AddItem
is dead, so the buy system gives weapons/ammo **FREE**. The real mechanic is a free ARMAMENT
(loadout) selector: the client menu queues weapons+ammo and `BuyArmament` applies them. Cash is
still tracked (the `cashforwin`/`cashforlose` cvars + a future sell zone) but never charged.

RE (recovered/buy/CBasePlayer.c): cash @+0x540 (clamped 0..30000, GiveCash @0x9310c); a pending
armament = ammo array @+0x554 (16 ints, rounds) + a weapon list @+0x594 (stride 0xc ×32: type/count/
gift). ResetArmament @0x93e88 (zeroes 0x70 ints), AddItemToArmament @0x93ea4 (first free slot),
AddAmmoToArmament @0x93ef4, BuyArmament @0x93f10 (each weapon → BuyWeapon or free AddItem; each
caliber → BuyAmmo), BuyAmmo @0x941e8, BuyWeapon @0x945bc (IsRestrictedWeapon @0x770c8 gate + the M61
special: type 0x18=24 is team-mode-only `1<TSGameRules` and capped 2/life via player+0x714, else free
PickUpWeapon). Functional reconstruction onto CBasePlayer + the cont.18 arsenal:
- `CashBased()` → false; `CBasePlayer::GiveCash` (clamped); members `m_iCash`/`m_iGrenadeBuys`/
  `m_armamentAmmo[16]`/`m_armament[32]` in player.h.
- `ResetArmament`/`AddItemToArmament`/`AddAmmoToArmament`/`BuyWeapon`/`BuyAmmo`/`BuyArmament`.
  `BuyWeapon` = M61 team-mode+cap-2 rule then free `GiveTSWeapon`; `BuyAmmo` = free `TSAddAmmo`.
- Console drivers in client.cpp (the faithful menu is cl_dll/`menuselect`, a SLICE): `tsbuy <type>`,
  `tsbuyammo <cal> <rounds>`, `tsarm <type> [count]`, `tsarmammo <cal> <rounds>`, `tsbuyarmament`,
  `tsresetarmament`.

**Verified RUNTIME (rcbot, temp probe reverted):** tm_avana — queue {beretta(2),uzi(3)} + 50×9mm,
GiveCash(1000)→1000, BuyArmament → arsenal=2 [2,3], 9mm reserve filled to the cap 80; M61 ×3 → 1,1,0
(grenadeBuys cap 2). ts_wyndham (DM) — armament still applies free (arsenal=2), M61 ×3 → 0,0,0
(team-mode block). No crash; clean regression (only the known ts_emitter/ts_particle Can't-inits).

**SLICE/deferred:** weapons are free (faithful — CashBased off); the category-by-bitmask
`IsRestrictedWeapon` over CTSMapGlobals' restriction isn't wired (default 0x1f allows all → allow-all
stub is faithful for stock maps); no cl_dll buy menu (console-driven); armament ammo treated as rounds
(no per-caliber mag-size table); no starting cash / cash-for-win-lose award yet; price field
(s_weapon_info+0xd8) not extracted (dead while CashBased off).

---

## 2026-06-21 (cont. 35) — CTSGameRules team subsystem: team ASSIGNMENT (the keystone)

**Done — `dlls/ts_teams.cpp`: a real team layer on CTSGameRules + auto-balance assignment.**
First piece of Track A. Players now get a `pev->team`, which is what the already-built objective/CTF
gating reads (cont.25/34) — so team mode is no longer just inheriting MP.

RE (from the .so): the team subsystem is a packed 0x8c-byte per-team struct ×4 (cash @+0x1c, scores
@+0x20/+0x24, name @+0x28, spawn classnames @+0x68/+0x88, model list @+0x38), gated on the TS
game-mode byte @+0 (2/3 = a team map). InitTeams @0x77f60 builds "Specialists;Mercenaries"
(models "spec"/"merc") for a team map; GetTeamIndex @0x79b04 = case-insensitive name→index (0-based);
GetTeamsNum @0x785a8; GetPlayerModel @0x79a08 / GetTeamCash @0x79aac read the per-team struct. The
faithful trigger for a player landing on a team is the client class-selection menu (cl_dll, NOT built).

Functional reconstruction (modeled naturally, NOT transcribing the byte struct; mirrors the SDK
`CHalfLifeTeamplay` team half, which is the same logic):
- `CTSGameRules` keeps `m_szTeamList` + parallel `m_teamNames[]/m_teamModels[]` + `m_numTeams`.
- `IsTeamMode()` (cached tri-state `m_iTeamMode`): a `ts_objective_manager` is present (the same DM
  proxy cont.26 used). Resolved lazily — worldspawn installs the rules before objective ents spawn.
  `IsTeamplay()` override now == `IsTeamMode()`; folds the old explicit objective-manager scan out of
  the cont.26 Think powerup gate.
- `InitTeams`/`RecountTeams`/`GetTeamIndex`(override)/`GetIndexedTeamName`(override)/`GetTeamID`
  (override → TeamID())/`TeamWithFewestPlayers` — mirror the SDK team subsystem.
- `AssignToTeam(player, name)`: set `pev->team` **0-based** (0 spec, 1 merc — the FGD/objective
  convention, NOT engine-1-based) + `m_szTeamName` + "model"/"team" infokeys + broadcast gmsgTeamInfo
  + gmsgScoreInfo. `PlayerSpawn` override: MP spawn, then (team mode, if not yet on a valid team)
  auto-balance onto `TeamWithFewestPlayers()` — idempotent across respawns/round restarts.
  `InitHUD` override sends gmsgTeamNames + everyone's team to a connecting client.

**Verified RUNTIME (rcbot, tm_avana, temp probe reverted):** 5 bots auto-balanced fewest-first →
Specialists{Chuck,Sean,sebastian}=3, Mercenaries{Alec,Matt}=2 (exact alternating 0,1,0,1,0 with
ties→lower index). 0 Can't init, no crash. DM regression (ts_wyndham): **0** assignments (IsTeamplay
false), powerup spawner still gated correctly, only the known ts_emitter/ts_particle Can't-inits, no
crash. Clean rebuild boot of tm_avana: probe gone, 0 Can't init, clean shutdown.

**SLICE/next (rest of Track A):** assignment is server auto-balance (faithful = client class menu,
cl_dll); the per-team **cash economy** (GetTeamCash/initial_cash @+0x1c, CashBased(), price
s_weapon_info+0xd8) + the **buy menu** server half (AddItemToArmament@0x83ea4 / BuyArmament@0x83f10 /
BuyWeapon@0x845bc) are not built; team **scores** still go through cont.31's `m_teamScore[]` (not the
faithful UpdateTeamScores/InitTeamScore); full `RestartRound`@0x67920 (round timer + game-mode member
+ InitObjectives/SetObjective) deferred; no friendly-fire (FPlayerCanTakeDamage) yet. Now that
pev->team is real, the **CTF/escape entities** (CTSFlag/CTSBriefcase/CTSBomb/CTSTeamEscape/CVipEscape/
CTSSellZone/CTSScoreGiver) are unblocked.

---

## 2026-06-21 (cont. 34) — CTSTrigger (TS brush trigger; team-gated, recheck delay)

**Done — `CTSTrigger` in `dlls/ts_objectives.cpp` (ts_trigger).** A reusable TS brush trigger.
RE (from the .so): Spawn @0x709b8 (brush SET_MODEL), WaitTouch @0x70ad8 — cooldown gate (param+0x41),
living-player check, attacker-team gate (GetTeamIndex vs +0x42), "Flag Carrier Only" spawnflag (0x1a0
& 8 → HaveFlag), FireTargets(target) + a sprintf'd HUD message, re-arm at time + max(0.5, nextcheck).
Functional reconstruction as `CTSTrigger : CTSObjective` (reuses cont.25's attackerteam + CanInteract):
KeyValue nextcheck (+ attackerteam); WaitTouch: cooldown → CanInteract(pev->team) → FireTargets +
TSHudMessageAll → arm next-allowed.

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — 0 stock placements):** a touch fired + armed the
cooldown (nextAllowed → t+2); an immediate re-touch was blocked. Factory `ts_trigger` exported; no
crash; clean regression (tm_avana, 0 Can't init).

**SLICE:** the "Flag Carrier Only" spawnflag needs the CTF flag system (not built) → ignored; the team
gate uses pev->team (now real as of cont.35).

**Session status:** ~25 TS classes reconstructed (the self-contained server-side gameplay is
essentially complete). Remaining = the big/client-entangled subsystems: faithful team mode + cash
(team assignment, SetTeamList, scores, the full RestartRound), the buy menu, the CTF entities
(CTSFlag/CTSBriefcase/CTSBomb/CTSTeamEscape/CVipEscape — need teams), CTSStunt (movement, client),
and the cl_dll side (visible time-dilation, particles CTSSharedEmitter/CTSEmitter/CTSParticle, HUDs).

---

## 2026-06-21 (cont. 33) — CTSSlowMotionPoint (the slow-motion bubble; powerup→GoSlow link)

**Done — `dlls/ts_slowmotionpoint.cpp`.** Un-SLICEs the deferred SlowMotionFlow link from
cont.14/15: an activated slow-motion powerup now actually drops nearby players into slow-motion.

RE (from the .so): CTSSlowMotionPoint (CreateSlowMotionPoint @0x69c60, ActivateSlow @0x663d0 sets
+0xe8 endtime/+0xec scale/+0xf4 bullets, WaitThink @0x71728). WaitThink polls every 0.5s: for each
living player, if `!InSlow()` and a TraceLine from the point to the player's eye is clear (LOS) →
`GoSlow(scale, bullets)` + track; at the end time → DeactivateSlow (restore tracked) + UTIL_Remove.
A runtime entity (0 FGD placements), created by the powerup/SlowMotionFlow.
Functional reconstruction on cont.15's CBasePlayer::GoSlow/InSlow:
- `CTSSlowMotionPoint : CBaseEntity`: ActivateSlow + WaitThink (LOS GoSlow) + DeactivateSlow (restore
  m_hSlowed[]). `TSCreateSlowMotionPoint(origin, scale, duration, bullets, owner)`.
- `CBasePlayer::UsePowerUp`: a slow-motion(0.6)/slow-pause(0.8) powerup spawns a bubble at the player
  (owner = activator, the powerup duration).

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — 0 stock placements):** a bubble at bot1 (owner)
with bot2 adjacent → bot2 `InSlow()=1 go=0.60`, owner bot1 `InSlow()=0` (skipped); at expiry the
bubble removed itself + restored bot2 (InSlow=0). No crash; clean regression (ts_theatre, powerups).

**SLICE:** the visible time-dilation MOVEMENT is still client-side (GoSlow sets server fields only);
the owner is skipped (stays "fast" = bullet-time feel) vs the .so's exact LOS set; no projectile slow.

---

## 2026-06-21 (cont. 32) — CTSDMHill (king-of-the-hill brush volume)

**Done — `dlls/ts_dmhill.cpp` (ts_dmhill).** A self-contained DM game-mode entity (no team/client
dependency). The `tm_`/team subsystem is the big remaining work, so this is a clean faithful
self-contained increment.

RE (from the .so): CTSDMHill (Spawn @0x71a8c brush SET_MODEL + SetSize, KeyValue @0x719b8, WaitTouch
@0x71be8 1433 bytes — tracks a single holder, awards `fragamt` (DM) / `fragamtteam` (team) frags to
pev->frags + gmsgScoreInfo + a "<netname> <message>" broadcast once held `nextcheck` seconds).
Functional reconstruction: a 0.5s `HillThink` polls the volume (`Intersects`) — exactly one living
player inside earns the timed award; 0 or 2+ (empty/contested) resets the king. KeyValue
nextcheck/fragamt/fragamtteam (+ message via the Target base).

**Verified RUNTIME (rcbot, ts_wyndham, SYNTHETIC — ts_dmhill has 0 stock placements):** a hill
instantiated around a sole bot awarded fragamt on the timer (frags 0→5). Factory `ts_dmhill`
exported; no crash; clean regression. (Build + synthetic only, like ts_timer/ts_customtarget — no
stock map exercises it; a hand-made test map would confirm in-map.)

**SLICE:** the .so's same-team-holders-don't-reset nuance simplified to "sole occupant".

---

## 2026-06-21 (cont. 31) — CTSGameRules::TeamWin + round restart (objective win ends the round)

**Done — the objective→win→round-restart chain.** Completes cont.25, where `CTSWinGiver::Use`
was a SLICE that only announced. The win now goes through the game rules (first piece of the
team subsystem).
- `CTSGameRules::TeamWin(team)`: bump `m_teamScore[team]`, announce the winner (TSHudMessageAll),
  schedule a restart 3s out (`m_flRoundRestart`; the .so TeamWin @0x68e00 uses param_1+0xc = time+3).
- `CTSGameRules::Think`: when `m_flRoundRestart` elapses, `TSRestartRound` respawns every player
  (SDK `respawn()`) + re-arms the objectives (re-Spawn each ts_wingiver/ts_hack) + announces.
- `CTSWinGiver::Use` → `TSGameRules()->TeamWin(m_winnerTeam)` (the inline announce removed).

**Verified RUNTIME (tm_avana, temp probe reverted):** firing the wingiver (winnerteam 2 = draw →
teams 0/1 unchanged, correct) scheduled a restart (pending=12.0); 3s later it fired → players
respawned + the wingiver re-armed (done=0). No crash; clean regression (ts_wyndham + tm_avana).

**SLICE:** faithful TeamWin awards victory/defeat cash + drives the round-time HUD; the full round
timer + the TS game-mode member + InitObjectives/SetObjective remain the team-subsystem TODO
(RestartRound @0x67920 ~1100 bytes, TeamWin @0x68e00 ~1564). No team ASSIGNMENT yet (scores track
the winnerteam the objective names, not per-player teams).

---

## 2026-06-21 (cont. 30) — CTSAmmoPack (drop ammo on death; consumable per-caliber pack)

**Done — `dlls/ts_ammopack.cpp`.** Completes the death-loot story: weapons (cont.27) + corpse
(cont.28) + now ammo. The dead player's per-caliber reserve drops as one consumable pack.

RE (from the .so): `CTSAmmoPack` (CreateAmmoPack @0xa1334 copies a 16-int per-caliber array;
Spawn @0xa13f4 model `ammopack.mdl` + `movetype=6` TOSS + `solid=2`; WaitTouch @0xa16c0 loops the
16 calibers calling `CTSGun::AddAmmo` and ZEROES each given entry — CONSUMABLE, vs the persistent
CTSGroundAmmo station cont.17). FUNCTIONAL reconstruction on the stock SDK (no CTSPhysicObject):
- `CTSAmmoPack : CBaseEntity` (ts_ammopack) carrying `m_ammo[16]`: MOVETYPE_TOSS → FallThink settle
  → SOLID_TRIGGER + WaitTouch. WaitTouch: for a TS-gun holder, `TSAddAmmo` each caliber, zero what
  was taken, UTIL_Remove when emptied. `TSCreateAmmoPack(owner, counts[16], origin, vel)`.
- `TSDropAmmoOnDeath(player)` (ts_gun.cpp): drop the gun's `m_ammoReserve[16]` as one pack.
- `CTSGameRules::PlayerKilled` now also calls it. `ammopack.mdl` precached at map load.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** killing a beretta-holding bot
dropped a pack carrying 9mm=45; a live tsgun holder touching it received the 9mm (pack m_ammo[1]
45→0) and the emptied pack was UTIL_Remove'd (deferred to frame end). No crash; clean regression
(factory `ts_ammopack` exported).

**SLICE:** no CTSPhysicObject bounce; one pack for the whole reserve (not per-mag); no thrown-pack.

---

## 2026-06-21 (cont. 29) — CTSShrapnel (grenade-blast fragments)

**Done — `CTSShrapnel` in `dlls/ts_grenade.cpp`.** The grenade explosion (cont.20 CM61Grenade)
now throws damaging fragments, matching the .so (a grenade Detonate spawns CTSShrapnel).

RE (from the .so): `CTSShrapnel : CTSPhysicObject` (Create @0x7bf9c, Spawn @0x7c0a0 with
`models/shrapnel.mdl` + `movetype=10` MOVETYPE_BOUNCE + `solid=2`, BounceTouch @0x7c200
TraceAttacks on contact, WaitThink @0x7c678 lifetime). FUNCTIONAL reconstruction on the stock SDK
(no CTSPhysicObject):
- `CTSShrapnel : CBaseEntity` (ts_shrapnel): MOVETYPE_BOUNCE + SOLID_BBOX; BounceTouch TraceAttacks
  a damageable hit (14 dmg DMG_BULLET, then remove) or bounces off the world ≤2× before expiring;
  1.5s FadeThink. `Create(owner, origin, velocity)`.
- `CM61Grenade::M61Tumble`/`M61Detonate` replace the stock TumbleThink→Detonate: tumble in flight,
  then at the fuse spawn 10 random-direction fragments + `CGrenade::Detonate` (stock area blast).
- `models/shrapnel.mdl` precached at map load (PrecacheAllTSWeapons; valve fallback model).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a thrown grenade detonated and
spawned the shrapnel burst (fragments bounce, several alive post-blast), no crash; clean
regression (factory `ts_shrapnel` exported).

**SLICE:** no CTSPhysicObject bounce, fixed 10 fragments/14 dmg, no fragment trail/sound.

---

## 2026-06-21 (cont. 28) — Death handling: CTSDeadCorpse + drop-arsenal-on-death

**Done — `dlls/ts_corpse.cpp` + a `CTSGameRules::PlayerKilled` hook.** Completes the on-death side
left open in cont.27 and adds the TS death body.

RE (from the .so): `CTSDeadCorpse` (Spawn @0x7040c, WaitThink @0x70460) + `TSCreateDeadCorpse`
(@0x60234). The corpse is `CBaseEntity::Create("…")` copying the player's origin/angles (rep movsl)
+ pev->model (+0xb8), modelindex (+0xb4), sequence/frame (+0x130/+0x1a8), `movetype = 6`
(MOVETYPE_TOSS @+0x108); WaitThink (0.5s) ramps renderamt down by `((fadeEnd-time)/(lifetime*0.667))*255`
then UTIL_Remove. Functional reconstruction:
- `CTSDeadCorpse : CBaseAnimating` (ts_corpse): MOVETYPE_TOSS + SOLID_NOT + kRenderTransTexture;
  `FadeThink` fades renderamt then removes (15s lifetime). `TSCreateDeadCorpse(player, lifetime)`
  copies model/origin/angles/sequence/frame(framerate 0 = frozen)/body/skin/colormap.
- `TSDropWeaponsOnDeath(player)` (ts_gun.cpp): scatter the whole arsenal into the world as
  CTSWorldGuns (random toss) — the on-death drop noted as SLICE in cont.27.
- `CTSGameRules::PlayerKilled` override: `CHalfLifeMultiplay::PlayerKilled` (scoring/death-notice),
  then drop weapons + spawn the corpse.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot given beretta+uzi then
force-killed (suicide TakeDamage) produced exactly **1 ts_corpse + 2 ts_worldgun**; no crash; clean
regression (factory `ts_corpse` exported).

**SLICE:** corpse copies the live pose at PlayerKilled time (no dedicated death-anim capture);
melee/no-world-model weapons aren't dropped; no gib/blood. The on-death weapon drop now means the
CTSWorldGuns are reachable in normal play (kill someone → their guns drop, press USE to grab).

---

## 2026-06-21 (cont. 27) — CTSWorldGun (player-dropped weapons: drop + USE-pickup)

**Done — `dlls/ts_worldgun.cpp`: the dropped-weapon-in-the-world entity.** Completes the weapon
lifecycle (map pickups via CTSGroundWeapon cont.11; player drops here).

RE (from the .so): `CTSWorldGun : CTSPhysicObject` carries a tsweapon_t (+0x88) + the full
s_weapon_status (14 ints); created by `CBasePlayer::DropWeapon`/`ThrowWeapon` -> `CTSGun::DropWeapon`
(removes from the arsenal) -> `CTSWorldGun::CreateWeapon`. `GetWeapon` (@0xa1198) -> `PickUpWeapon`;
`WaitTouch` (@0xa1250) gates pickup on `pev->button & IN_USE` (0x20) (or auto for the flag, type 0x1d).
FUNCTIONAL reconstruction on the stock SDK (CM61Grenade-style; no CTSPhysicObject):
- `CTSWorldGun : CBaseEntity` carrying type + loaded clip. Spawn: w_model + MOVETYPE_TOSS;
  FallThink settles on FL_ONGROUND -> SOLID_TRIGGER + WaitTouch. WaitTouch: living player pressing
  IN_USE -> TSPickUpWeapon + UTIL_Remove.
- `TSCreateWorldGun(type, clip, origin, vel)` = the drop side.
- `CTSGun::RemoveActiveWeapon(type, clip)` (@DropWeapon 0x8094c): shift the active slot out of the
  arsenal (requires >=2 weapons — no unarmed state, SLICE), wield the next; free `TSDropActiveWeapon`/
  `TSPickUpWeapon` (the latter re-adds via GiveTSWeapon + restores the carried clip).
- `dropweapon` ClientCommand drops the active weapon forward (250u + player velocity).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot holding {beretta, uzi} dropped
the uzi (type 3, clip 32; arsenal 2->1) -> ts_worldgun spawned + settled (SOLID_TRIGGER); a forced
USE-touch handed the uzi back (holds weapon_tsgun, world gun removed). No crash; clean regression
(factory `ts_worldgun` exported).

**SLICE:** no CTSPhysicObject bounce, carries type+clip only (the shared caliber reserve stays with
the dropper), no on-death auto-drop, no flag auto-pickup, no throw arc.

---

## 2026-06-21 (cont. 26) — CTSGameRules::PutPowerUp (dynamic deathmatch powerup spawning)

**Done — the dynamic powerup spawner.** Un-SLICEs the `CTSGameRules::PutPowerUp` (@0x9797c)
niche left in cont.13. DM maps now spawn powerups dynamically even without ts_powerup placements.
- `CTSGameRules::PutPowerUp(type)`: collect `info_player_deathmatch` spots (cap 32), pick a
  random one, skip if a `ts_powerup` is already within 128u, else create a powerup there via a
  new `TSCreatePowerUpAt(type, origin)` free helper (ts_powerup.cpp; wraps
  `CTSPowerUp::CreatePowerUp` + UTIL_SetOrigin so it drops from the spot).
- `CTSGameRules::Think` override (the rules object now runs TS think): `CHalfLifeMultiplay::Think()`
  then, in DM only, roll `GetRandomPowerup(0x1ff)` every ~20s and PutPowerUp it (1s retry on
  fail). Mirrors the .so Think powerup block (`if mode<2 … every 20s`). DM gate = `!IsTeamplay()`
  (which as of cont.35 == the objective-manager proxy).

**Verified RUNTIME (temp probe reverted):** ts_wyndham (DM) spawned a random powerup every 20s
(t=1 kungfu, t=21 slowmo, t=41 kungfu); tm_avana (objective map) correctly skipped (0 spawns).
No crash; clean regression.

---

## 2026-06-21 (cont. 25) — Objective subsystem (ts_hack/wingiver/manager/ptr) — team-mode maps

**Done — `dlls/ts_objectives.cpp`: the 4 objective entity types the `tm_` maps place.** Loading
a team-mode map (tm_avana) showed 7 Can't-inits: ts_hack(2), ts_objective_manager(1),
ts_objective_ptr(1), ts_wingiver(3). All cleared.

RE (from the .so): these are point/brush entities tied into a CTSGameRules team subsystem
(`TeamWin` @0x68a08 1564 bytes, `ShowObjectiveOnHud`, `AddObjectiveInList`) that ISN'T built —
the interaction "who is hacking" + the win scoring/round-restart are gamerules-driven. Built a
FUNCTIONAL standalone reconstruction instead:
- `CTSObjective` (base @0x8c2b0): `attackerteam` keyvalue (+0x68, -1=any) + CanInteract(team)/
  Freeze(bool). (.so Freeze also calls ShowObjectiveOnHud — SLICE.)
- `CTSObjectiveManager` (@0x8b7bc, ts_objective_manager): objptargeta..h / messagea..d /
  targeton{timeleft,specdead,mercdead} keyvalues; `Reset()` HUDs the messages + FireTargets the
  8 objptargets (matches @Reset 0x8b850).
- `CTSObjectivePointer` (@0x8bb44, ts_objective_ptr): per-team objmsg{spec,merc,team3,team4,all}
  (the visible HUD arrow is cl_dll — server stub).
- `CTSHack` (@0x8cccc, ts_hack): brush; SET_MODEL + SOLID_TRIGGER; an attacker-team living player
  in the volume (HackTouch) drives a `hackingtime` countdown (WaitThink, 1s cadence like the .so);
  on 0 → fire pev->target + HUD pev->message + Freeze. SLICE: faithful start-hack is gamerules-driven.
- `CTSWinGiver` (@0x8bf10, ts_wingiver): Use'd `callsnumber` times → declares `winnerteam` (HUD).
  SLICE: faithful Use → CTSGameRules::TeamWin(team, victory/defeat cash, …) + round restart.

**Verified RUNTIME (tm_avana, temp probe reverted):** 2 hack / 3 wingiver / 1 manager / 1 ptr
spawn (0 Can't init, was 7); keyvalues parse (hack time=10 attackerteam=-1, wingiver team=2
calls 1); a forced-hacker hack counted 10→0 and completed (done=1); wingiver Use → declares the
winner (done=1). tm_corporate also 0 Can't init. No crash; clean.

**Next:** the real payoff is the **CTSGameRules team subsystem** (TeamWin/team assignment/scores/
RestartRound) — it turns these standalone entities into an actual scored team game mode and
unblocks the rest of the tm_ maps (ts_briefcase/ts_bomb/ts_flag CTF). Still also: the buy menu,
CTSSharedEmitter (cl_dll), the visible slow-motion (cl_dll).

---

## 2026-06-21 (cont. 24) — Shotgun fidelity: 8-pellet count + mossberg/sawedoff fire-types

**Done — closed the two known weapon data gaps.**
- s_weapon_info gains `iPellets` (bullets-per-shot, `s_weapon_info+0x24`): **8 for the five
  shotguns** (benelli/usas/spas/mossberg/sawed-off), 1 otherwise (extracted from each
  Write_<weapon>). `StandardFire` uses it instead of the hardcoded `(ft==PUMP)?6:1` guess —
  which was both wrong (8, not 6) and missed usas/sawedoff (not pump fire-type but 8 pellets).
- Fixed two fire-types the OLD extract_weapons.py mis-read as 0 (reading the Write_ funcs
  directly gives the truth): **mossberg 0→1 (pump), sawedoff 0→8 (semi, twin-barrel)**.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** mossberg fireType=1 pellets=8,
sawedoff fireType=8 pellets=8, usas fireType=6 pellets=8, beretta control pellets=1; all fire
without crash. Clean regression.

**Note on the bigger picture:** the weapon engine is now functionally complete (arsenal,
all fire-types, akimbo, fire-mode/burst, per-caliber ammo, correct pellets). The remaining TS
gameplay systems are all LARGE/interconnected or client-entangled: the buy menu (cash economy
@BuyWeapon checks CashBased()/weapon price s_weapon_info+0xd8/player cash +0x540 + a client
menu UI), the team/objective subsystem (CTSGameRules::TeamWin @0x68a08 is 1564 bytes atop
SetTeamList/UpdateTeamScores/RestartRound/InitTeamScore — all the ts_wingiver/ts_hack/
ts_briefcase entities live in tm_ maps needing it), the networked particles (CTSSharedEmitter,
needs cl_dll), and the visible slow-motion time-dilation (needs cl_dll). Next agent: pick the
team foundation (biggest gameplay unlock, server-side) or the buy menu server half.

---

## 2026-06-21 (cont. 23) — Per-caliber ammo (CTSGun::AddAmmo + CTSGroundAmmo wiring)

**Done — the faithful per-caliber ammo system.** Un-SLICEs the per-slot generic reserve +
the CTSGroundAmmo 9mm-only refill; ammo is now pooled per caliber and shared across same-
caliber weapons (a single beretta + akimbo berettas draw from one 9mm pool).

RE findings (from the .so):
- Each weapon names a CALIBER byte at `s_weapon_info+0x8` (extracted from each Write_<weapon>):
  0 none(melee/grenade), 1 9x19, 2 .45, 3 SHELLS, 4 5.56, 5 .50AE, 6 7.62, 7 5.7, 8 50BMG,
  9 10mm, 10 .22, 11 .454, 12 .32, 13 .45LC. The gun holds 16 per-caliber reserves at `+0xd4`.
- `compile_ammo_info` (@0x6f51c) builds a per-caliber table (stride 0x38): +0x10 = magazine
  size, **+0x20 = max reserve (rounds)** — 9mm 80, .45 90, shells 90, 5.56 150, 7.62 170,
  50BMG 200, … (full table extracted).
- `AddAmmo(caliber,count)` (@0x808ac): `reserve[caliber] = min(reserve+count, max[caliber])`.
- `ReloadWeapon` (@0x84a14): `take = min(clipSize - clip, reserve[caliber])` → reserve is in
  ROUNDS, drawn into the clip. (Settled the rounds-vs-mags question.)
- `CTSGroundAmmo::WaitTouch` (@0xa2f40): loops the 16 calibers calling `AddAmmo(gun, cal,
  m_mags[cal])` — the FGD `magsN` keyvalues add raw rounds to caliber N (the station persists,
  topping up on every touch; real maps set magsN = 3–5).

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iCaliber` (the +0x8 byte, full table); `kAmmoMax[16]`
  + `TSAmmoMax(caliber)`.
- `ts_gun.cpp` CTSGun: **replaced the per-slot `m_arsenalReserve[]` with per-caliber
  `m_ammoReserve[16]`**. `AddAmmo` method (capped). Spawn/AddWeapon seed the caliber reserve
  (clip*3); the akimbo upgrade adds the 2nd pistol's rounds to the shared pool; Reload draws
  from `m_ammoReserve[caliber]`; MultiplyClips (inf-clip powerup) tops the active caliber.
  Free `TSAddAmmo(player, caliber, count)`.
- `ts_groundammo.cpp` WaitTouch: per-caliber `TSAddAmmo(player, cal, m_mags[cal-1])` (was the
  9mm-only SLICE).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta → clip 15 / 9mm reserve
45; emptied clip → Reload drew 15 from the 9mm pool → 15/30; 2nd beretta → akimbo (type 8) +
SHARES the 9mm pool (reserve 30+15=45); +m16 → 5.56 reserve 54, 9mm unchanged (separate pools);
40× AddAmmo(1,3) capped at max 80. Clean regression on ts_supertimor_d3 (6 ground-ammo packs):
0 Can't init, 0 crashes.

**SLICE/deferred:** starting reserve = clip*3 (the faithful buy menu / BuyArmament sets the real
loadout); calibers 12/13 (skorpion/contender) have no ground-ammo magN; no client ammo HUD;
grenade (caliber 0) still uses its own m_iClip count (no caliber reserve).

---

## 2026-06-21 (cont. 22) — Fire-mode system (auto/semi/burst/pump + SwitchFireMode)

**Done — the faithful trigger engine.** Auto/semi/burst/pump is now decided by a per-slot
FIRE-MODE (orthogonal to fire-type, which still picks the fire *function*) — replacing the
old fire-type-based auto/semi approximation.

RE findings (from the .so):
- `PrimaryAttack` (@0x81a20): melee/grenade/katana/flag dispatch by fire-type FIRST (outside
  the mode system); the bullet/akimbo path then gates on the fire-mode byte `m_pActiveItem+2`:
  mode 0 full-auto (held), 1 semi (per-press; dblfire powerup → held), 2 burst (set count on
  press, return — fired by WeaponIdle), 3 pump (held), 4 akimbo-alt (per-press), 5 akimbo-both
  (held). Burst count = `(weaponType != 0x17) + 2` → **3 rounds, 2 for the MAC-10 (0x17)**.
- `WeaponIdle` (@0x854dc): while firemode==2 and burst count>0 and the idle timer elapsed →
  `Fire()` + count-- (this completes a tapped burst after the trigger is released).
- `StandardFire` (@0x824a4): burst rate = 0.03 between rounds, 0.2 after the last.
- `SwitchFireMode` (@0x8169c): cycle the mode byte to the next bit set in the weapon's mode
  MASK at `s_weapon_info+0xc`, modes 0..5 wrapping. Default mode (`TakeWeapon`): auto if
  offered, else pump, else burst, else semi. Masks extracted from each `Write_<weapon>`
  (`movl $imm,0xc(%eax)`): e.g. beretta=2 (semi), uzi=3 (auto+semi), m16=6 (semi+burst,
  defaults burst), mp5sd/mp5k/mac10=7 (auto+semi+burst), akimbos 18/33.

Functional reconstruction:
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iFireModes` (the +0xc mask, full table extracted);
  `enum tsfiremode_mode_t`; `TSDefaultFireMode(mask)`.
- `ts_gun.cpp` CTSGun: per-slot `m_arsenalMode[]` + active `m_fireMode` + `m_burstCount`
  (saved/loaded by SelectSlot; defaulted in Spawn/AddWeapon/akimbo-upgrade). PrimaryAttack
  rewritten to gate by m_fireMode → `Fire()` (dispatches by fire-type: AkimboFire or the new
  `StandardFire`, factored from the old inline bullet code). WeaponIdle drives the burst.
  `SwitchFireMode()` + free `TSSwitchFireMode` + the `tsfiremode` ClientCommand.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** default modes correct for all
masks (beretta→1 semi, uzi→0 auto, m16→2 burst, mp5sd→0 auto); `SwitchFireMode` cycles only
the available modes (beretta stays 1; uzi 1↔0; m16 1↔2; mp5sd 1→2→0); m16 burst fired exactly
3 rounds per tap (clip 18→15, burst 3→0). No crash; clean regression (only the known
`ts_emitter`/`ts_particle` Can't-inits remain).

**SLICE/deferred:** burst sustain depends on the SDK ItemPostFrame/WeaponIdle cadence (a held
burst fires via PrimaryAttack, a tapped one finishes via WeaponIdle); no client-side fire-mode
HUD indicator (the .so writes `mode+1` to player+0x49a per weapon); pump (mode 3) doesn't yet
play a pump-action animation; the dblfire→semi-auto-continuous interaction is approximated.

---

## 2026-06-21 (cont. 21) — Akimbo (fire-types 4/5): dual-wield upgrade + AkimboFire

**Done — akimbo dual-wielding.** Un-SLICEs fire-types 4 (combinable) & 5 (gold-only),
the last unhandled PrimaryAttack fire-type.

RE findings (from the .so):
- `IsWeaponAkimbo(t)` = `get_weapon_info(t)->iFireType==4`; `IsOnlyAkimbo(t)` = `==5`
  (objdump @0x7019c/0x701d4 — both just compare the +0x348 fire-type byte).
- Akimbo PAIRING fields in s_weapon_info: **+0xc8** = the akimbo variant a single pistol
  upgrades into (-1 if none); **+0xcc** = the single base of an akimbo (= self for
  non-akimbo). Read from each `Write_<weapon>` (`movl $imm,0xc8/0xcc(%eax)`; the table is
  .bss so file reads are garbage — must read the Write_ funcs). Pairs: 2↔8 (beretta),
  3↔16 (uzi), 9↔10 (mk23), 14↔30 (fnh), 17↔37 (skorpion); gold(21) is ft-5, no single.
- `TakeWeapon` (@0x80a04): owning the single + picking up an identical 2nd → `local_12`
  becomes the +0xc8 akimbo → combines in place (clip = sum of both barrels). `AkimboFire`
  (@0x829c0) splits the clip into left/right barrels (m_pActiveItem+0x1c/+0x20), alternates
  the firing barrel (one per IN_ATTACK/IN_ATTACK2) from a ±5u right-vector muzzle offset;
  rate = flFireRate, or fixed 0.1 in alternating fire-mode.

Functional reconstruction (on the cont.18 single-clip arsenal):
- `ts_weapons.{h,cpp}`: s_weapon_info gains `iAkimboType`/`iSingleType`; set in
  compile_weapons_info. Added `TSFT_AKIMBO_ONLY=5`.
- `CTSGun::AddWeapon`: if you already hold the single + it has an akimbo upgrade + don't
  hold the akimbo → upgrade the slot in place (SelectSlot first so m_iClip is live, then
  type→akimbo, clip = single's rounds + a 2nd pistol capped at the doubled clip, reserve
  += a 2nd pistol, re-Deploy). Slot count stays the same (not a new slot).
- `CTSGun::AkimboFire(wi)` (dispatched for ft 4/5): full-auto-feel (fires while held — no
  IN_ATTACK2 plumbed), `m_akimboSide ^= 1` each shot → ±5u v_right muzzle offset, one
  bullet/clip-- per shot, flSpread/flDamage/flFireRate (+ DoubleFireRate powerup halving).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** gave a bot beretta, then a
2nd beretta → upgraded in place to **type 8 (beretta_akimbo), clip 30** (15+15 capped),
reserve 60, arsenal count stayed 1. Held trigger → AkimboFire fired (clip 30→26→16→6, then
reload from reserve → 26, side toggled 0↔1), active item = the gun. No crash; clean
regression (0 crashes; only the known `ts_emitter`/`ts_particle` Can't-init remain).

**SLICE/deferred:** single active clip (not a true L/R barrel split); no IN_ATTACK2
"fire both barrels at once" (firemode 5 path); no per-barrel reload; the akimbo's separate
view-model akimbo animation events are not wired. Fire-mode/burst (`SwitchFireMode`) and the
buy menu remain the next weapon-engine items.

---

## 2026-06-21 (cont. 20) — CM61Grenade (grenade fire-type 2 throws a grenade)

**Done — `dlls/ts_grenade.cpp` CM61Grenade + the grenade fire-type.** Un-SLICEs
fire-type 2 (was falling back to bullets) + makes the grenade powerup useful.
- The faithful CM61Grenade is `CM61Grenade : CTSGrenade : CTSPhysicObject` (TS physics
  base classes, not built) with event-driven creation + a custom WaitThink (traces 64u
  ahead for near-impact detonation). FUNCTIONAL reconstruction onto the stock SDK
  `CGrenade` instead (which already does arc/bounce/timed-detonate): `CM61Grenade::Throw`
  replicates `CGrenade::ShootTimed` but with `w_m61.mdl` (so no models/grenade.mdl
  precache dependency) — MOVETYPE_BOUNCE, `CGrenade::BounceTouch`, `TumbleThink`→
  `Detonate` at dmgtime, gravity 0.5/friction 0.8. Free `TSThrowM61(pevOwner, src, vel,
  fuse, dmg)`.
- `CTSGun::PrimaryAttack` dispatches TSFT_GRENADE (2) → new `GrenadeThrow(wi)`: one throw
  per press, decrement clip, toss from gun-pos+16 forward at 800u + player velocity, 4s
  fuse, damage = table flDamage*4 (~120). (m61 table dmg 30 is the per-pickup value; the
  explosion is area damage.)

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot with the M61 threw
100 grenades over 30s (valid entities, clip decrementing); TumbleThink switches to
Detonate at dmgtime (model-independent: `if (dmgtime <= time) SetThink(Detonate)`), so
they arc/bounce/explode. No crash; clean regression.

**SLICE/deferred:** no CTSPhysicObject physics, no impact-trace early detonation, no
throw event/animation; akimbo (fire-types 4/5) still falls back to bullets.

---

## 2026-06-21 (cont. 19) — Powerup→weapon wiring: MultiplyClips + DoubleFireRate

**Done — two now-easy powerup effects, faithful on the cont.18 arsenal.** Completes
the powerup→weapon connection.
- `CTSGun::MultiplyClips()` (@0x71308, the infinite-clip powerup): doubles the active
  arsenal slot's reserve (>= a full resupply, capped ~6 clips). Free `TSMultiplyClips
  (player)` wraps it (GetPlayerTSGun). `PlayerAddPowerUp`'s inf-clip branch now calls it
  (was a SLICE clip-refill).
- `CTSGun::PrimaryAttack`: `if (DoubleFireRate(m_pPlayer)) flRate *= 0.5` — the
  double-firerate powerup now halves the fire interval (the accessor already flips true
  while the dblfire powerup is active; just wired into the rate).

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** beretta inf-clip reserve
45→90 (doubled); dblfire baseRate 0.100 → rate 0.050 (halved), 105 fires at the doubled
rate. No crash; clean regression.

---

## 2026-06-21 (cont. 18) — CTSGun weapon arsenal (the items-array, modern reconstruction)

**Done — the player's weapon_tsgun now holds a multi-weapon ARSENAL.** A clean
MODERN reconstruction of the original items-array (the faithful one is a 0x38-byte
s_weapon_status items[] @CTSGun+0x114 with m_pActiveItem@0xcc / m_pActiveInfo@0xd0 +
weapon/ammo-group masks + akimbo pairing — heavy byte layout, reconstructed from
intent, not transcribed).
- `dlls/ts_gun.cpp` CTSGun: added parallel arrays `m_arsenalType/Clip/Reserve[12]` +
  `m_arsenalCount` + `m_active`. `m_weaponType` now tracks the active slot (so the
  fire-type dispatch + Deploy are unchanged).
- `FindSlot(type)` / `AddWeapon(type)` (@AddItem__6CTSGun: add a slot or resupply,
  then wield) / `SelectSlot(slot)` (@SelectWeapon__6CTSGun: save the wielded clip,
  load the new slot's, re-Deploy if held).
- `Spawn` seeds the arsenal with the placed type (clip full, reserve = 3 clips).
- `PrimaryAttack` unchanged (operates on m_iClip = the active slot's clip). `Reload`
  now refills m_iClip from the active slot's own reserve (not DefaultReload).
- `GiveTSWeapon(player, type)`: if the player already holds weapon_tsgun, AddWeapon
  to it; else create + pick up. (Helper `GetPlayerTSGun` walks m_rgpPlayerItems.)
- `CTSGroundWeapon::GetWeapon`: dropped the "one TS weapon at a time" guard — picking
  up a ground weapon now ADDS it to the arsenal.
- `tsnextweapon` ClientCommand → `TSCycleWeapon` cycles the active slot.

**Verified RUNTIME (rcbot, ts_wyndham, temp probe reverted):** a bot built a 3-weapon
arsenal (beretta→+uzi→+knife), `tsnextweapon` cycled 0→1→2→0 through all three, each
deploying its own view model with its own clip/reserve (beretta 15/45, uzi 32/96,
knife 18/54). PER-SLOT CLIP PERSISTS: the bot fired the uzi (clip 32→30) and the count
survived switching away+back. No crash; clean regression (0 Can't init, 0 crashes).

**SLICE/deferred:** no akimbo pairing (single active slot — the faithful engine wields
two for fire-types 4/5), no weapon/ammo-groups, no fire-mode/burst state, no buy menu
(`BuyWeapon`/`BuyArmament`); the per-caliber ammo reserve is still per-slot generic (not
wired to CTSGroundAmmo's calibers). The grenade/akimbo PrimaryAttack fire-types still
fall back to bullets.

---

## 2026-06-21 (cont. 17) — CTSGroundAmmo (ts_groundammo refill pack, 16 placements)

**Done — `dlls/ts_groundammo.cpp` CTSGroundAmmo : CBaseEntity.** A map-placed ammo
pack; lifecycle mirrors CTSPowerUp (Spawn→Fall→OnGround settle→WaitTouch/WaitThink),
recovered from the .so (Spawn/OnGround tails truncated by Ghidra noreturn at
UTIL_SetOrigin → read via objdump):
- Spawn: model `models/ammopack.mdl`; movetype = MOVETYPE_FLY (wall, spawnflag 16) or
  MOVETYPE_TOSS (floor); solid SOLID_BBOX; SetThink(Fall) + nextthink time+0.1.
- Fall: settle on FL_ONGROUND → OnGround (or PutOnWall for the wall variant) + drop
  sound. OnGround: SOLID_TRIGGER, MOVETYPE_FLY (stop), clear EF_NODRAW + EF_DIMLIGHT,
  SetTouch(WaitTouch)+SetThink(WaitThink). WaitThink idles (nextthink+1).
- KeyValue: wduration → m_respawnTime; mags1..mags11 (parse the index off "mags") →
  m_mags[0..10] (per-caliber magazine counts: 9x19/.45/SHELLS/5.56/.50AE/7.62/5.7/
  50BMG/10mm/.22/.454).
- WaitTouch: a visible pack refills a touching ALIVE player who holds a TS gun. The
  pack PERSISTS (tops players up when they have room; not consumed on pickup — matches
  the .so, which has no hide/remove in WaitTouch).
- PutOnWall: SLICE = settle-in-place stub.

**SLICE:** faithful WaitTouch loops the 16 ammo-type slots calling `CTSGun::AddAmmo(
caliber, mags)` on the active gun (the per-caliber reserve = part of the unbuilt
weapon items-array; gate is m_pActiveItem id == 0x10). Here it sums the mags and tops
up the held weapon's **9mm** reserve via `CBasePlayer::GiveAmmo(n, "9mm", _9MM_MAX_CARRY)`.

**Verified RUNTIME (rcbot bots, ts_supertimor_d3 = 6 placements), temp probe (reverted):**
6 settled (`Can't init ts_groundammo` gone); a weapon_tsgun-holding bot touched a pack
(14 mags) → GiveAmmo topped up 9mm by 2 then returned -1 (full) on repeat = correct
capped refill-station behaviour. No crash (gdb -batch, no CTSGroundAmmo in backtrace).
Final clean regression boot: 0 `Can't init`, 0 crashes.

---

## 2026-06-21 (cont. 16) — CTSGun fire-type dispatch (semi/auto/shotgun/melee)

**Done — weapons now fire faithfully by fire-type.** Un-SLICEs "everything is identical
semi-auto bullets".
- `scripts/extract_weapons.py` extended: also pulls per-weapon fire-rate (0x18), base
  spread (0x28), recoil (0x34). Regenerated the table.
- `ts_weapons.{h,cpp}`: s_weapon_info gains flFireRate/flSpread/flRecoil; added
  `enum tsfiretype_t` (0/8 semi, 1 pump, 2 grenade, 3 knife, 4/5 akimbo, 6 auto-shotgun/
  rifle, 7 sniper, 9 SMG-auto, 10 katana, 11 flag).
- `CTSGun::PrimaryAttack` dispatches by iFireType: melee (3/10) → new MeleeAttack()
  hull-trace (knife 48u/katana 64u) + TraceAttack(DMG_CLUB), no bullets; full-auto (9/6)
  fires while held; semi (0/8/7/1 + SLICE akimbo/grenade) one shot per press
  (m_afButtonPressed); pump shotgun (1) → 6 pellets. Uses real flSpread + flFireRate.
- SLICE: grenade(2)/akimbo(4/5) fall back to bullets (need throw entity / items-array,
  where the faithful fire funcs read fire-mode@m_pActiveItem+2 / burst@+8).

**Verified (rcbot, ts_wyndham, probe reverted):** beretta ft=8 SEMI 43 fires (one/press)
vs uzi ft=9 AUTO 201 (held); benellim3 ft=1 6-pellet; knife/katana hull-trace MELEE hit
func_breakable/worldspawn, no bullets. No crash. **Data gaps:** mossberg(26)/sawedoff(33)
fire-type extracted as 0 (are shotguns); pellet count fixed 6; m61 rate/spread suspect.

---

## 2026-06-21 (cont. 15) — CTSSlowMotion entity + GoSlow/InSlow + GlobalSlowMotion2

**Done — slow-motion STATE layer + `dlls/ts_slowmotion.cpp` (ts_slowmotion brush zone,
11 placements).**
- Slow virtuals `GoSlow`/`InSlow`/`GiveSlowMul` (found via CBasePlayer vtable relocs:
  +0x14 GoSlow, +0x18 InSlow). As **CBasePlayer methods**: GoSlow(scale,bullets) sets
  m_flSlowMotionGo (0x168) + m_flSlowMotionTarget (0x170) + m_bSlowBullets; InSlow() =
  (go!=1||target!=1); both init 1.0.
- `CTSGameRules::GlobalSlowMotion2(scale,bullets)` + SetGlobalSlowMotion: GoSlow every
  player, throttled ~0.5s (members 0x560/0x564/0x568/0x56c). SLICE: no projectiles.
- CTSSlowMotion : CBaseEntity (replaces old draft): Spawn (brush trigger; setup beyond
  classname/model/size INFERRED // VERIFY); KeyValue duration/slowamt; WaitTouch (GoSlow
  toucher, track m_hPlayers[33]); ActivateSlow/DeactivateSlow; WaitThink (0.5s poll);
  CheckPlayersDistance (drop beyond m_flRadius from Center()); Use; Save/Restore.

**Verified (rcbot, ts_wyndham=4):** 4 zones; Use() no crash; GlobalSlowMotion2(0.5,1) →
InSlow()=1 go=0.50; (0,0) → InSlow()=0 go=1.0. No crash.

**SLICE/deferred (visible time-dilation, client-entangled):** actual slow MOVEMENT
(client pmove — no cl_dll); projectile slow; SlowMotionFlow (@0x86bb0, powerup→GoSlow);
InterpolateSlowMotion (@0x8910c); motion-blur HUD; cmd_globalslow; Spawn/m_flRadius inferred.

---

## 2026-06-21 (cont. 14) — Powerup activation state machine (UsePowerUp/UpdatePowerUpState)

**Done — held timed powerups ACTIVATE + expire.** From UsePowerUp (@0x85980),
UpdatePowerUpState (@0x85748), DeactivateSlowMotionFlow (@0x86a9c):
- CBasePlayer::UsePowerUp() (ts_player.cpp): `usepowerup` command. Alive + holding +
  none active → m_iPowerups = m_iHeldPowerup, expire = m_flPowerupTime + time; sends
  gmsgTSPowerUpTime + gmsgTSMotionBlur. UpdatePowerUpState() per-frame in PreThink:
  apply slow fields (slowmo 0.6, slowpause 0.8) / expire + clear.
- `usepowerup` ClientCommand (confirmed via .so @0x9d838). Players +0x175/0x17c/0x180.

**Verified (ts_theatre):** bot held slowmo (dur 3) + UsePowerUp → m_iPowerups=1,
SlowMotion()=1, scale 0.60 for 3s, expired + cleared. No crash. (Note: activation sets
m_flSlowMotionScale, NOT GoSlow — the powerup→GoSlow link is SlowMotionFlow, deferred.)

---

## 2026-06-21 (cont. 13) — CTSPowerUp + PlayerAddPowerUp (the 328-placement pickup)

**`dlls/ts_powerup.cpp` CTSPowerUp : CBaseAnimating (ts_powerup, 328 placements).** Full
lifecycle (Spawn + OnGround TRUNCATED by Ghidra noreturn at UTIL_SetOrigin — real tails
via objdump; Spawn's `nextthink = time+0.1` missed → never settled, caught by "22
spawned 0 pickups"). Spawn→Fall→OnGround(SOLID_TRIGGER,SetTouch(WaitTouch))→WaitTouch
(pickup gate: instant mask 0x1e6 always, timed only if none held; re-roll+respawn).
SetupPowerUp: spawnflags = the random MASK (GetRandomPowerup), pwuptype forces exact.
**PlayerAddPowerUp** (@0x87bc8): instant real (armor/kungfu/lowgrav/health/grenade→
GiveTSWeapon(M61) SLICE/infclip SLICE), timed held. Members 0x72c/0x73c. VERIFIED
(ts_theatre=22): 22 spawned all 8 types, 6 pickups, gate works, no crash.

---

## 2026-06-21 (cont. 12) — Full 38-weapon table + CTSGun by type; gdb debugging

`scripts/extract_weapons.py` → full s_weapon_info table (0x44 name, 0x132 v_model,
0x172 p_model, 0x1f2 w_model, 0x232 anim-ext, 0x2c6 fire snd, 0x04 clip, 0x1c damage,
0x348 fire-type; cont.16 added 0x18/0x28/0x34). CTSGun parameterized by m_weaponType;
ground weapon gives real type. CWorld::Precache → UTIL_PrecacheOtherWeapon at map load.
VERIFIED (ts_wyndham): type 25→v_knife dmg40, type 3→v_uzi dmg22. Two GDB-found crashes
fixed (PRECACHE_EVENT map-load; DeathNotice ItemInfoArray). **Use gdb** for crashes.

---

## 2026-06-21 (cont. 11) — CTSGroundWeapon (weapon pickups) + tsweapon_t fix

tsweapon_t index = (lea offset − 0x7d30)/0x3DC, MATCHES FGD tsweaponid. beretta=2, uzi=3,
knife=25, idx 29=flag. ts_groundweapon.cpp (65 placements): SLICE gives weapon_tsgun;
respawn after wduration. VERIFIED (ts_wyndham).

---

## 2026-06-20 (cont. 10) — CTSGun recon + weapon data layer (vertical-slice start)

**CTSGun is a subsystem:** ONE unified CBasePlayerWeapon (id 16) morphing by tsweapon_t.
**tsweapon_t enum (38, compile_weapons_info @0x6fe28):** 0 melee,1 glock18,2 uzi,
3 uzi_akimbo,4 benellim3,5 m4,6 mp5sd,7 mp5k,8 beretta_akimbo,9 beretta,10 mk23,
11 mk23akimbo,12 usas12,13 desert,14 ak47,15 fnh,16 aug,17 skorpion,18 barrett,19 pdw,
20 spas12,21 gold,22 glock22,23 mac10,24 m61,25 knife,26 mossberg,27 m16,28 ruger,
29 fnhakimbo,30 bull,31 m60,32 sawedoff,33 katana,34 sealknife,35 contender,
36 skorpion_akimbo,37 flag.
FAITHFUL CTSGun engine (items array m_pActiveItem@0xcc/m_pActiveInfo@0xd0/items[]@0x45,
fire-type dispatch s_weapon_info+0x348, akimbo/firemode/zoom) replaces // SLICE.

**Bot test rig (working, reusable):** ~/Containers/hlds/hlds-linux64-test:
metamod_amd64.so (Metamod-R) as gamedll (liblist.gam → it); config.ini=`gamedll
dlls/hl.so`; plugins.ini=`linux addons/metamod/dlls/rcbot_mm_amd64.so`. <rig>/rcbot/*;
min_bots 2. Boot → rcbot bots = real CBasePlayers. Temp StartFrame hook can give/select/
teleport/fire on a bot (gate on a player existing — bots connect ~t>6-8; pulse IN_ATTACK
via m_afButtonPressed to test semi vs auto; slide an entity onto a bot to force touch).
Revert hooks after. Probe output → <rig>/ts/qconsole.log with -condebug.

---

## 2026-06-20 (cont. 9) — TSHudMessageOne + CBasePlayer TS extensions (step 1)

`dlls/ts_utils.{h,cpp}` TSHudMessageOne. CBasePlayer TS member block (player.h):
m_iPowerups (0x724), m_iHeldPowerup (0x728), m_flPowerupTime (0x730), m_iKungFu (0x74e),
m_bLowGravity (0x74f) + accessors. (cont.13 +0x72c/0x73c; cont.14 +0x175/0x17c/0x180;
cont.15 +0x168/0x170 GoSlow/InSlow.)

**The core is a monolith.** Sequencing done: (1) player block+accessors; (2) CTSGun
base; (3) PlayerAddPowerUp + CTSPowerUp; (4) UsePowerUp/UpdatePowerUpState; (5) GoSlow/
InSlow + GlobalSlowMotion2 + CTSSlowMotion; (6) CTSGun fire-type dispatch; (7)
CTSGroundAmmo. NEXT: the faithful weapon items-array (m_pActiveItem/m_pActiveInfo +
s_weapon_status + AddAmmo + PickUpWeapon/BuyWeapon armament + buy menu — un-SLICEs
akimbo/grenade/burst-fire, the per-caliber ammo, and the inf-clip/grenade powerup
branches) and/or the visible slow-motion time-dilation (needs cl_dll); objective/CTF
entities (CTSHack/CTSBriefcase/CTSBomb/CTSFlag/CTSWinGiver) for team modes. Fill
CTSGameRules methods as needed.

---

## 2026-06-20 (cont. 8) — CTSGameRules skeleton installed + runtime-verified

**No CTSPlayer class** — TS extends stock CBasePlayer. `CTSGameRules : CHalfLifeMultiplay`
is the live g_pGameRules (InstallGameRules). GetRandomPowerup(mask) reconstructed.
Caveat: tm_ teamplay maps use MP base; revisit when TS team logic lands.

## 2026-06-20 (cont. 7) — CTSMapGlobals runtime-verified
get_weapon_restriction() = 0x1f ^ spawnflags. (ts_trinityfall).

## 2026-06-20 (cont. 6) — CTSModel + CTSFog; FIRST in-engine runtime verification
`dlls/ts_model.cpp` cycler-style prop (858 placements). Decompile TRUNCATED by Ghidra
noreturn at UTIL_SetSize; real body via objdump (Ghidra adds 0x10000 vs nm).
**Runtime rig:** 64-bit ReHLDS (QUIC) at ~/Containers/hlds/hlds-linux64-test. Boot:
`LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH timeout -s INT 30 ./hlds_linux -game ts -condebug
+developer 1 +sv_lan 1 +maxplayers 4 +map <map>`. SIGINT not KILL. `Can't init <class>`
= negative control. With -condebug, ALERT(at_console) probe → <rig>/ts/qconsole.log.

## 2026-06-20 (cont. 5) — CTSCustomTarget built (point relay; no stock map uses it).
## 2026-06-20 (cont. 4) — DECISION: functional/source-faithful, NOT byte drop-in.
   ts_utils TSHudMessageAll + ts_timer CTSTimer. glibc 2.14.
## 2026-06-20 (cont. 3) — CTSSlowMotion draft (superseded by cont.15).
## 2026-06-20 (cont. 2) — gnv2demangle.py (1788/2426 typed sigs) + DecompExport.java.
## 2026-06-20 (cont.) — Vino OK'd reconstruction; Ghidra pipeline (decompile.sh).
## 2026-06-20 — Scaffolding: fork halflife-updated@64bit-demo; reference/; tooling;
   work-list RECONSTRUCTION.md (60 TS classes). Builds via hlds64-buildchain image.

**Conventions:** NO Co-Authored-By trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
