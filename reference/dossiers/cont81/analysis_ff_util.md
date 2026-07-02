# Group: TS_util.cpp free functions (13 funcs) — server utility layer

Ground truth: ff_*.c decompiles in this scratchpad + objdump on `reference/ts_i386.so`
(lock-free). Addresses below are nm file addresses (Ghidra = +0x10000).

Shared facts established for the whole group:

- **gmsgTSCustomWeaponParams wire name = `"CustomWP"`, size -1** (registration at
  0xc8367: `REG_USER_MSG("CustomWP", -1)`; string @0xfe55b; GOT slot 0x167bac,
  `gmsgTSCustomWeaponParams` @0x1034c4). Client handler = client.dll FUN_1004fb50
  (gmsg_TSCustomWeaponParams.c): sscanf `"%i %f %f %f %f %i"` → client weapon-info copy
  fields [0x28],[0x30],[0x34],[0x1c],[0xd8], then sets bit0 of `this+0x10` (a "custom
  weapons in use" HUD flag).
- **s_weapon_info tunable-param quintet** (same five offsets everywhere in this group;
  meanings from save_weapon_value's own header comment string
  `"//values are : |weapon id|accuracy|view kick|accuracy kick|damage|cost|"`):
  `+0x28` accuracy (our `flSpread`), `+0x30` view kick (our `flViewKick`), `+0x34`
  accuracy kick (our `flRecoil`), `+0x1c` damage (our `flDamage`), `+0xd8` cost (int,
  buy price — NOT yet in our `s_weapon_info`; our `slotCost` is +0xd0, a different field).
  `+0x44` = weapon display name (our `pszName`).
- **CTSGameRules members**: `+0x5884` (byte) = "weapon params customized" flag — set by
  AdjustSpecificWeapon and adjust_weapon_value, read by `CTSGameRules::InitHUD`
  (@0x69134, gate `cmpb $0,0x5884` at 0x691c8) to decide whether to send the custom
  params to a joining client. `+0x5888` = `health_absorb` (float), `+0x588c` =
  `armor_absorb` (float, ours: `m_flArmorAbsorb`, ts_gamerules.h:195).
  `+0x684 + idx*0x48` = per-player vote block, stride 0x48: map-vote string (0x40 bytes,
  ours `m_voteMap[32][32]` — **stride mismatch: original is 64-char names**, VERIFY),
  then FF vote byte @+0x40 (= abs 0x6c4 for idx 0, ours `m_voteFF`), TP vote byte @+0x41
  (0x6c5, ours `m_voteTP`), 6 pad bytes.
- **CBasePlayer members** (all transient; none in members.tsv, names VERIFY):
  `+0x160` (int) — when nonzero PlayerIsCheating short-circuits false; zeroed on
  disconnect right next to clearing FL_FAKECLIENT → almost certainly the **is-bot /
  fakeclient flag** (VERIFY).
  `+0x208` = `CBaseEntity* m_pSlowedEnts[32]` — entities this player has slowed
  (players AND projectiles: knife/grenade/shrapnel/worldgun/flag). `+0x288` (char) =
  count, capped at 0x1f, reset in TSInit (0x82e5f `movb $0,0x288`). NB our
  player.h:410 documents `m_hSlowPlayers[32]` as "0x188" — the binary says **0x208/0x288**
  (SlowMotionFlow disasm 0x86eff/0x86ea4); fix the provenance comment.
  `+0x534` (float) — set to -4.0 on disconnect (VERIFY name).
  `+0x770` (float) — per-player respawn/in-game timer; -4.0 = not in game; dead players
  with `0x770 >= 0` still count as "active" in TSCountPlayers (VERIFY name,
  ~m_flRespawnTime).
  `+0x7b9` (byte) = ours `m_bTheOne` (player.h:417) — cleared on disconnect.
  `+0x9b8`, `+0xb30` — zeroed on disconnect (VERIFY).
  `+0xb48` = `char m_szTeamModel[…]` — the player's team/model string; TeamID() returns
  it (vtable +0xd0); first byte NUL'd on disconnect; compared/forced in
  ClientUserInfoChanged ("model" key).
- Vtable slots used via `this->vtbl` (VTABLES.md): `+0xb4` IsAlive, `+0xc8` IsPlayer,
  `+0xd0` TeamID.
- `DAT_00106657` = "" (empty string); `DAT_00109d6a` = "/";
  `DAT_00107ee0` = "weapons/knife/knife_hitbody.wav".

---

## 1. UTIL_EntInArray @0x8e944

1. SIGNATURE: `int UTIL_EntInArray(CBaseEntity **pArray, int nCount, CBaseEntity *pEnt)`
   (signatures-gcc2.txt: `UTIL_EntInArray(CBaseEntity * *, int, CBaseEntity *)`).
2. BEHAVIOR: plain linear search (the decompile's mess is just gcc2's 4-way unroll):
   return the first index `i` in `[0, nCount)` with `pArray[i] == pEnt`, else `-1`.
   `nCount < 1` → `-1`. No side effects, no globals.
3. CALLERS (disasm-verified via PLT): `RemoveEntityFromSlowedArray` (this group) and
   `CBasePlayer::SlowMotionFlow` @0x86bb0 (dedup check before appending to the
   `+0x208` slowed array at 0x86f06).
4. OURS: none. Our SlowMotionFlow (dlls/ts_player.cpp:213-260) dedups via the
   indexed-by-player-slot `m_hSlowPlayers[i-1]` cache instead of a packed array+search,
   and only tracks players.
5. RECONCILE: write fresh into the future `dlls/TS_util.cpp` (trivial). It becomes
   load-bearing only when the packed slowed-entity array (`+0x208`/`+0x288` model) is
   adopted for projectile slow-motion parity; adopting that array is playtest-gated
   (touches SlowMotionFlow).

## 2. WeaponIdFromString @0x8e9c0

1. SIGNATURE: `int WeaponIdFromString(char *psz)` — returns tsweapon_t id or -1.
2. BEHAVIOR: for id 0..0x25 (38 weapons), `strcasecmp(psz, get_weapon_info(id)->name /*+0x44*/)`;
   first match returns id; none → -1. (Decompile is the same loop unrolled by 2.)
3. CALLERS: **NONE in ts_i386.so** — no PLT stub, no relocation, no direct call
   (verified: `readelf -r` has no entry for it; ts.dis has only its own PC-thunk).
   Dead/unreferenced in the Linux build. VERIFY whether mp.dll (Windows) references it
   (its symbol survives only because the .so isn't stripped); plausibly it served a
   console command (`adjustweapon <name>`?) that was compiled out.
4. OURS: no equivalent (no name→id lookup anywhere; grep for
   strcasecmp/pszName/WeaponIdByName is empty).
5. RECONCILE: write fresh in TS_util.cpp for inventory parity; mark as unreferenced
   (matches original). Behavior-preserving, no playtest impact.

## 3. AdjustSpecificWeapon @0x8ea3c

1. SIGNATURE: `void AdjustSpecificWeapon(tsweapon_t id, float accuracy, float viewkick,
   float accuracykick, float damage, int cost)`.
2. BEHAVIOR: gate `id < 0x26` and `get_weapon_info(id)->name[0] != 0`. Writes the five
   tunables: `+0x28`=accuracy, `+0x30`=viewkick, `+0x34`=accuracykick, `+0x1c`=damage,
   `+0xd8`=cost (no clamping here, unlike adjust_weapon_value). Then
   `SendWeaponParams(id, NULL)` (broadcast to all), `UTIL_ClientPrintAll(HUD_PRINTTALK=3,
   UTIL_VarArgs("%s behaviour changed\n", get_weapon_info(id)->name))`, and sets
   gamerules byte `+0x5884 = 1` (custom-weapons flag → future joiners get params in
   InitHUD).
3. CALLERS: **NONE in ts_i386.so** (same verification as WeaponIdFromString — no reloc,
   no call). Dead code in the Linux build; presumably the handler for a removed/Windows
   `adjustweapon` console command (pairs naturally with WeaponIdFromString). VERIFY vs
   mp.dll.
4. OURS: none. Weapon stats are compile-time table values (dlls/ts_weapon_info.cpp);
   no runtime adjust path at all.
5. RECONCILE: write fresh in TS_util.cpp (uses SendWeaponParams below). Unreferenced,
   behavior-preserving.

## 4. SendWeaponParams @0x8eb98 — the CustomWP emitter

1. SIGNATURE: `void SendWeaponParams(tsweapon_t id, CBaseEntity *pOnly)`.
2. BEHAVIOR: gate `id < 0x26` && weapon name non-empty. Builds a **string** payload in a
   64-byte buffer (buf[63] forced NUL):
   `sprintf(buf, "%i %f %f %f %f %i", id, info->0x28, info->0x30, info->0x34, info->0x1c, info->0xd8)`
   i.e. `id accuracy viewkick accuracykick damage cost`. Then (disasm 0x8ec3d-0x8ec96):
   - `pOnly != NULL` → `MessageBegin(MSG_ONE=1, gmsgTSCustomWeaponParams, NULL, pOnly->pev->pContainingEntity)`
   - else → `MessageBegin(MSG_ALL=2, gmsgTSCustomWeaponParams, NULL, NULL)`
   then `WriteString(buf)`, `MessageEnd()`. One WRITE_STRING — the whole wire format is
   the ASCII line (matches the client parser exactly).
3. CALLERS: `AdjustSpecificWeapon` (broadcast) and `InformClientsOnWeaponsParams`
   (per-weapon loop). Both in this file.
4. OURS: nothing — "CustomWP" is not registered server-side (dlls/UserMessages.cpp has
   no TS custom-params message) nor hooked client-side. This is a MISSING swap-contract
   message: an original client connecting to our server with customized weapons expects
   CustomWP; our client on an original custom server will drop/ignore it.
5. RECONCILE: write fresh in TS_util.cpp; register `gmsgTSCustomWeaponParams =
   REG_USER_MSG("CustomWP", -1)` with the other TS messages; client hook
   `MsgFunc_CustomWP` writing the client weapon-info copy (we share one table TU —
   apply to `g_weaponInfo` fields flSpread/flViewKick/flRecoil/flDamage/cost) + set the
   HUD "custom weapons" flag. Requires adding the `+0xd8 cost` field to our
   `s_weapon_info` (buy-menu price — check ts_buy.cpp pricing source first, VERIFY:
   if ts_buy has its own price table, it must be unified with +0xd8 for adjust to affect
   prices as the original does). Wire format byte-faithful = drop-in critical; the
   sending path itself is behavior-preserving to add (only fires when custom weapons are
   used).

## 5. InformClientsOnWeaponsParams @0x8eb14

1. SIGNATURE: `void InformClientsOnWeaponsParams(CBaseEntity *pOnly)`.
2. BEHAVIOR: announcement + full-table sync.
   - `pOnly == NULL` → `TSHudMessageAll(0x2d, 0xc3, 0x2d, 8, 100, "This server is using
     custom weapons.")` (green-ish 45,195,45; effect 8; holdtime 100);
     else `TSHudMessageOne(same, pOnly->pev)`.
   - Then `SendWeaponParams(i, pOnly)` for i = 0..0x25 (all 38 slots; unsent slots are
     filtered inside SendWeaponParams by the empty-name gate).
3. CALLERS (disasm-verified): `CTSGameRules::InitHUD` @0x691d8 — **only when gamerules
   +0x5884 flag is set** — sends the joining player (pOnly=player) the custom table;
   and `cmd_loadweapons` @0xb59ed (game.cpp group) — after `adjust_weapon_value`
   succeeds, broadcast (pOnly=NULL).
4. OURS: none. Our `CTSGameRules::InitHUD` (dlls/ts_teams.cpp:301) sends team roster +
   round time only.
5. RECONCILE: write fresh in TS_util.cpp; add the `if (m_bWeaponParamsCustom)
   InformClientsOnWeaponsParams(pPlayer)` call into our CTSGameRules::InitHUD (add the
   gamerules flag member, provenance +0x5884). Depends on the gmsg + cmd_loadweapons
   (game.cpp group) landing together. Behavior-preserving when no custom weapons file
   is loaded.

## 6. adjust_weapon_value @0x8eca4 — load + apply a weapons config file

1. SIGNATURE: `int adjust_weapon_value(char *pszFile)` — 1 = handled/loaded, 0 = no file.
2. BEHAVIOR:
   - `GetGameDir(dir)`; `sprintf(path, "%s%s%s", dir, "/", pszFile)`; `fopen(path, "rb")`;
     NULL → return 0.
   - Per line (`fgets`, 1024 max, copied into a 256 buffer): right-trim trailing
     whitespace via `__ctype_b` (isspace), zero-fill the rest of the 256 buffer; skip
     empty lines; skip any line containing `"//"` **anywhere** (comment filter).
   - Line contains `"armor_absorb"` → `sscanf(p+12, " %f ", &v)`; clamp `v < 0 → 0`;
     store gamerules `+0x588c` (armor absorb fraction).
   - Else contains `"health_absorb"` → `sscanf(p+13, " %f ", &v)`; clamp ≥0; store
     gamerules `+0x5888`.
   - Else weapon line: `sscanf(line, " %i %f %f %f %f %i ", &id,&acc,&vkick,&akick,&dmg,&cost)`.
     **Fewer than 6 fields → `ALERT(at_console=1, "You need to run saveweapons
     again.\n")` and return 1 immediately** (aborts mid-file but still reports
     handled). Valid `id < 0x26` + name non-empty → store: `+0x28 = max(acc, 0.001f)`
     (fc_0_0010000000474974513 = the documented "accuracy cant go under 0.001" clamp),
     `+0x30 = vkick`, `+0x34 = akick`, `+0x1c = dmg`, `+0xd8 = cost` (only accuracy is
     clamped).
   - EOF: `fclose`; `UTIL_ClientPrintAll(HUD_PRINTTALK=3, UTIL_VarArgs("weapons
     behaviour changed by %s\n", pszFile))`; gamerules `+0x5884 = 1`; return 1.
   - Does NOT broadcast SendWeaponParams itself — the caller (cmd_loadweapons) follows
     with `InformClientsOnWeaponsParams(NULL)`.
3. CALLERS: `cmd_loadweapons` (game.cpp — the `customweapons` cvar/command
   path; strings "customweapons", example "customweapons weapons_custom.txt").
4. OURS: none (only the cvar shell exists: dlls/ts_cvars.cpp:40 registers
   `customweapons` FCVAR_SERVER but nothing reads it).
5. RECONCILE: write fresh in TS_util.cpp. Constants that matter: 0.001 accuracy floor,
   ≥0 clamps on both absorbs, the exact ClientPrintAll strings, the early-return-1 on a
   short line, comment filter = substring "//" anywhere. Needs `m_flHealthAbsorb`
   added to CTSGameRules (+0x5888 — currently missing; our TakeDamage only implements
   armor_absorb; VERIFY where the original reads +0x5888 — likely
   CBasePlayer::TakeDamage health scaling — before wiring). Server-only file I/O,
   behavior-preserving until a server op actually uses `customweapons`.

## 7. save_weapon_value @0x8f044 — write the weapons config file

1. SIGNATURE: `int save_weapon_value(char *pszFile)` — 1 = written, 0 = fopen failed.
2. BEHAVIOR:
   - Same path build (`gamedir + "/" + file`); `fopen(path, "w")`; NULL → 0.
   - Writes the fixed header comment block (the exact 421-byte string in .rodata:
     "//===…\n//values are : |weapon id|accuracy|view kick|accuracy kick|damage|cost|…
     //-accuracy cant go under 0.001 … 'customweapons' cvar … weapons_custom.txt …").
   - For id 0..0x25 with non-empty name:
     `sprintf("//%s\n%i %f %f %f %f %i\n\n", name(+0x44), id, +0x28, +0x30, +0x34, +0x1c, +0xd8)`
     then fwrite (each weapon = a comment line with its name + its tunables line).
   - Footer: `sprintf("//===…\n//Misc game variables\narmor_absorb %f\n", gamerules->0x588c)`.
     **Only armor_absorb is saved; health_absorb is parseable by adjust_weapon_value but
     never written** (real asymmetry in the original — keep it).
   - `fclose`; `UTIL_ClientPrintAll(3, UTIL_VarArgs("weapons behaviour saved to %s\n", pszFile))`;
     return 1.
3. CALLERS: `cmd_saveweapons` @game.cpp (console command `saveweapons`; its error string
   "ERROR: specify a file!; example: \"saveweapons weapons.txt\"" is in the binary).
4. OURS: none.
5. RECONCILE: write fresh in TS_util.cpp, byte-identical strings (the header block is
   user-visible file content). Ships with cmd_saveweapons/cmd_loadweapons (game.cpp
   group). Behavior-preserving.

## 8. TSCountPlayers @0x8f238

1. SIGNATURE: `int TSCountPlayers(char *pszTeam, bool bOnlyActive)`.
2. BEHAVIOR: `teamFilter = (pszTeam[0] != '\0')` (inline compare vs "" @DAT_00106657).
   For i = 1..maxClients: `p = UTIL_PlayerByIndex(i)`; skip NULL; skip `!p->IsPlayer()`
   (vcall +0xc8). If teamFilter: skip when `strcasecmp(pszTeam, p->TeamID()) != 0`
   (vcall +0xd0 — TeamID returns the +0xb48 team/model string). If bOnlyActive: skip
   when `!p->IsAlive()` (vcall +0xb4) **AND** `p->+0x770 < 0.0f` — i.e. dead players
   still awaiting respawn (0x770 ≥ 0) DO count; fully out-of-game players (0x770 = -4,
   set on disconnect) don't. Returns the count.
3. CALLERS: `CTSTeamEscape::WaitTouch` @0x7d575, args `TSCountPlayers(teamString, true)`
   (pushw $1) — the escape-objective "how many escapers must reach the zone" population.
4. OURS: inlined substitute in dlls/ts_escape.cpp:83-92 — counts
   `IsAlive() && GetTeamIndex(TeamID()) == attacker_team`. TWO deltas: (a) uses team
   INDEX equality instead of case-insensitive string match (equivalent, fine); (b) does
   NOT count dead-but-respawn-pending players (`0x770 >= 0`), so our escape-completion
   denominator is smaller whenever a teammate is dead mid-round. Gameplay-relevant in
   respawn-enabled escape rounds. VERIFY the exact meaning of +0x770 before porting.
5. RECONCILE: move the ts_escape.cpp inline into a real `TSCountPlayers(char*, bool)` in
   TS_util.cpp with the original's semantics (string TeamID match + the 0x770 dead-alive
   exception), and call it from CTSTeamEscape::WaitTouch. Playtest-gated (changes escape
   round completion thresholds).

## 9. TSPrecache @0x8f320

1. SIGNATURE: `void TSPrecache(void)`.
2. BEHAVIOR (order preserved in the decompile):
   1. 50 fixed PRECACHE_SOUNDs: player/superjump-land,land,dive,airwheel,headshot,
      headshot2,kevlarhit,block,splat,bodyhit1-4,die,die1-3,pain1-4; misc/hacking,hacked,
      failed,successful,gogogo; debris/ric1-5,cement,cement1-6,gravel1-4,metal1-5;
      weapons/shell,sshell.
   2. Weapon-table loop i=0..0x25: for each `get_weapon_info(i)`, precache each
      non-empty string field: MODELS `+0xe8`, `+0x132` (v_model), `+0x172` (p_model),
      `+0x1b2`, `+0x1f2` (w_model); SOUNDS `+0x2c6` (fire), `+0x306` (second sound,
      likely silenced fire). NB the original precaches **five** model fields per weapon —
      our struct carries only v/p/w (+0x132/+0x172/+0x1f2); +0xe8 and +0x1b2 are two
      more model slots (VERIFY: recover via extract_weapons.py).
   3. Post-loop fixed set: weapons/pistol-empty, insert-shell, goslow.wav,
      weapons/whizz1-5, player/closecombat, player/kungfuhit, weapons/weapondrop,
      weapons/items, weapons/draw, weapons/switch, items/9mmclip1,
      weapons/generic_reload, explo/explode + explode1-4 (sounds);
      models/generic_clip.mdl, ammopack.mdl, bullet.mdl, long_bullet.mdl, pellet.mdl;
      `UTIL_PrecacheOther("m61_grenade")`, `("shrapnel")`, `("ts_flag")`;
      weapons/knife/knife_miss, knife_hit, knife_hitbody (DAT_00107ee0);
      weapons/katana/hit-body, miss1, miss, hit, hit1.
3. CALLERS: `CWorld::Precache` (disasm-verified) — map-load time, unconditional.
4. OURS: split/renamed: `PrecacheAllTSWeapons()` (dlls/ts_weapons.cpp:16) covers the
   weapon-table loop + much of the fixed set, but (a) it is called from
   `CTSGroundWeapon::Precache`/`CTSGun::Precache` and via
   `UTIL_PrecacheOtherWeapon("weapon_tsgun")` in CWorld::Precache (world.cpp:698)
   rather than a direct TSPrecache call; (b) misses vs original: player/block.wav,
   player/splat.wav, die/die1-3, misc/hacking/hacked/failed/successful/gogogo,
   debris/cement*/gravel*/metal*, weapons/shell/sshell, weapons/pistol-empty,
   player/closecombat, player/kungfuhit, weapons/items, weapons/draw, weapons/switch,
   items/9mmclip1, weapons/generic_reload, explo/explode*,
   models/generic_clip/bullet/long_bullet/pellet.mdl, UTIL_PrecacheOther
   shrapnel/ts_flag/m61_grenade (ours precaches shrapnel.mdl but not the entity chain),
   katana/miss1 + hit1; (c) contains many EXTRA precaches the original does elsewhere
   (CTSGun::Precache sprites/sil-sounds/events) — audit will flag them as misplaced.
5. RECONCILE: rename/consolidate — create `TSPrecache()` in TS_util.cpp with the
   original's exact list+order, call it from CWorld::Precache, and shrink
   PrecacheAllTSWeapons to whatever CTSGun::Precache originally owned (cross-check
   reference/coverage/precache.tsv per function). Missing precaches are latent
   Host_Error bugs (runtime PRECACHE after spawn on late use of e.g. player/splat.wav).
   Mostly behavior-preserving; duplicates are tolerated by the engine.

## 10. TSPlayerDisconnect @0x8f94c

1. SIGNATURE: `void TSPlayerDisconnect(CBasePlayer *pPlayer)`.
2. BEHAVIOR:
   - `pPlayer->DropFlag(false)` (CTF drop, no throw).
   - `idx = ENTINDEX(pev->pContainingEntity)`; if `idx < 0x20`: zero the 0x48-byte
     per-player vote block `gamerules + 0x684 + idx*0x48` (18 dwords: map-vote name +
     FF/TP votes).
   - pev resets: `solid = SOLID_NOT`, `deadflag = DEAD_DEAD(2)`, `takedamage = 0`,
     `flags &= ~0x2000` (**clear FL_FAKECLIENT**), `flags |= 0x80000000`
     (**set FL_DORMANT**), `effects = 0x80` (EF_NODRAW), `frags = 0`, `health = 0`.
   - member resets: `+0x160 = 0` (bot flag), `+0xb30 = 0`, `+0x770 = -4.0f` (out of
     game), `+0xb48[0] = '\0'` (team/model string), `+0x9b8 = 0`, `+0x534 = -4.0f`,
     `+0x7b9 = 0` (m_bTheOne).
   - `CTSGameRules::CallCheckDeads(gamerules)` — a departure can end a team round.
3. CALLERS: `CHalfLifeMultiplay::ClientDisconnected` (disasm-verified; TS routes the
   SDK disconnect through it).
4. OURS: fragmented: dlls/client.cpp:148 does the DropFlag; stock
   `CHalfLifeMultiplay::ClientDisconnected` (multiplay_gamerules.cpp:424) does
   logging + RemoveAllItems but none of the TS resets; CallCheckDeads exists
   (ts_gamerules.h:113) but is not called on disconnect; the vote block is never
   cleared on disconnect (a rejoining slot inherits the old player's votes — live bug);
   m_bTheOne not cleared (juggernaut-mode leak).
5. RECONCILE: write `TSPlayerDisconnect` in TS_util.cpp and call it from our
   ClientDisconnected override (move the DropFlag out of client.cpp into it, matching
   the original call chain). Clear m_voteMap/m_voteFF/m_voteTP[idx], set the pev fields,
   clear m_bTheOne, call CallCheckDeads. VERIFY the +0x534/+0x770/+0x9b8/+0xb30 member
   identities as they get named (members_re curation). Playtest-gated lightly
   (disconnect-during-round outcomes change: round can now end when the last enemy
   leaves — that is the original behavior).

## 11. TSFoundRandomEntity @0x8fa64

1. SIGNATURE: `CBaseEntity *TSFoundRandomEntity(char *pszClassname)` (decompile shows
   `void` and a bogus `FUN_0009fa94()` call — **decompile-failed artifact**: 0x8fa94 is
   the middle of its own loop, not a callee. Disasm-verified full logic.)
2. BEHAVIOR (objdump 0x8fa64-0x8fadf): chain
   `p = UTIL_FindEntityByClassname(p, pszClassname)` collecting results into a 32-slot
   stack array (stops at NULL or 32, `cmp $0x1f,%esi`); none → NULL;
   else return `arr[RANDOM_LONG(0, n-1)]` (engine vcall +0x168 = enginefuncs index 90 =
   pfnRandomLong).
3. CALLERS: **NONE in ts_i386.so** (no reloc/PLT/direct call — dead code in the Linux
   build, same class as WeaponIdFromString/AdjustSpecificWeapon). Likely a helper for
   random spawn/powerup placement superseded by GetRandomPosition / TSSelectSpawnSpot.
   VERIFY vs mp.dll.
4. OURS: none by name; our powerup/spawn randomization uses different paths.
5. RECONCILE: write fresh in TS_util.cpp, unreferenced, for inventory parity.
   Behavior-preserving (dead).

## 12. PlayerIsCheating @0x8fae0

1. SIGNATURE: `bool PlayerIsCheating(CBasePlayer *pPlayer, char *pszInfoBuffer)`.
2. BEHAVIOR: if `pPlayer->+0x160 != 0` (bot flag) → false. Else
   `sscanf(g_engfuncs.pfnInfoKeyValue(pszInfoBuffer, "cl_lw"), "%i", &v)`; return
   `v != 1`. I.e. a human client with cl_lw (client-side weapon prediction) not exactly
   "1" is "cheating" (TS's fire events assume cl_lw 1).
3. CALLERS (disasm-verified):
   - `CHalfLifeMultiplay::ClientUserInfoChanged` @0x7b289: if true →
     `pfnSetClientKeyValue(idx, GetInfoKeyBuffer(edict), "cl_lw", "1")` — the server
     FORCES cl_lw back to 1 (no kick). (The same function then guards team/"model"
     changes against +0xb48 with "* Can't change team to '%s'\n".)
   - `CBasePlayer::TSInit` @0x82ec2: calls it and **discards the result** (benign
     no-op call in the original; keep for source parity, it has no side effects).
4. OURS: none — our `ClientUserInfoChanged` (multiplay_gamerules.cpp:107) only does
   `SetPrefsFromUserinfo`. No cl_lw enforcement anywhere. Drop-in relevant: original
   servers enforce cl_lw=1; ours doesn't, so prediction-desync configs are possible.
5. RECONCILE: write fresh in TS_util.cpp + add the ClientUserInfoChanged enforcement in
   our gamerules override (and the vestigial TSInit call). Requires the bot flag
   (+0x160) or `FBitSet(pev->flags, FL_FAKECLIENT)` — VERIFY which our bots set.
   Behavior-preserving for normal clients (cl_lw defaults to 1).

## 13. RemoveEntityFromSlowedArray @0x8fb4c

1. SIGNATURE: `void RemoveEntityFromSlowedArray(CBaseEntity *pEnt)`.
2. BEHAVIOR: for every connected player (1..maxClients, `UTIL_PlayerByIndex`, IsPlayer
   vcall +0xc8) with slowed-count `+0x288 > 0`: loop `count` times calling
   `UTIL_EntInArray(player+0x208, count, pEnt)` and NULL-ing each found slot
   (`arr[idx] = 0`). Holes are left (no compaction, count unchanged) — the array
   tolerates NULL entries. Purpose: a slowed projectile (knife/grenade/shrapnel/
   worldgun/flag) being destroyed/reclaimed is scrubbed from every player's slowed
   cache so restore-on-deactivate never touches a freed entity — the original has no
   EHANDLE for these.
3. CALLERS (disasm-verified, 8): `CTSFlag::Destroy`, `CTSWorldGun::Fall`,
   `CTSWorldGun::GetWeapon`, `CTSWorldGun::WaitThink`, `CTSKnife::Use`,
   `CTSKnife::WaitThink`, `CM61Grenade::WaitThink`, `CTSShrapnel::WaitThink`.
4. OURS: none — dlls/ts_player.cpp:202 SLICE comment explicitly defers projectile
   slow-restore; our caches are EHANDLE-based (m_hSlowPlayers, ts_slowmotionpoint
   m_hSlowed) which self-null on entity destruction, making the scrub unnecessary FOR
   SAFETY — but restore-semantics differ (original restores the projectile's physics
   via RestorePhysics on deactivate; a silently-nulled EHANDLE just skips).
5. RECONCILE: port together with the projectile-GoSlow feature (already ledger-tracked):
   adopt the packed `m_pSlowedEnts[32]`/`m_nSlowedEnts` (+0x208/+0x288) model and add
   the explicit scrub calls at the 8 caller sites (mimic the original mechanism —
   memory `mimic-original-mechanism-for-dropin`). Playtest-gated (slow-motion
   projectile behavior).

---

## Cross-cutting notes / VERIFY list

- VERIFY (refs pass): whether mp.dll references the three Linux-dead functions
  (WeaponIdFromString / AdjustSpecificWeapon / TSFoundRandomEntity) — binmap/live trace.
- VERIFY member names: CBasePlayer +0x160 (is-bot), +0x534, +0x770, +0x9b8, +0xb30;
  fix player.h:410's wrong "0x188" provenance for the slowed array (binary: 0x208/0x288).
- VERIFY: our m_voteMap[32][32] vs the original's 0x48-stride (64-char map names) —
  behavioral only if map names >31 chars, but audit-level mismatch.
- VERIFY: gamerules +0x5888 health_absorb read-site (TakeDamage?) before adding
  m_flHealthAbsorb.
- VERIFY: s_weapon_info +0xe8 and +0x1b2 model fields (precached by TSPrecache; not in
  our struct) and +0x306 second sound (silenced?) — recover via extract_weapons.py.
- VERIFY: ts_buy.cpp price source vs +0xd8 cost — must be the same storage for
  customweapons to reprice the buy menu as the original does.
- File-consolidation: all 13 belong in a new `dlls/TS_util.cpp` (+ the CustomWP
  registration in UserMessages.cpp and the client MsgFunc). cmd_loadweapons /
  cmd_saveweapons themselves are game.cpp-group work these functions plug into.
