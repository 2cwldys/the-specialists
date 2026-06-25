# HANDOFF — for the next Claude picking up mod-ts

You're resuming the reconstruction of **The Specialists ("ts")** as a functional,
source-faithful HL mod. This is the continuation brief — not a tutorial.

## Read these first (in order)
1. **`CLAUDE.md`** (repo root, auto-loaded on this machine) — mission, method,
   tooling, conventions. The stable reference.
2. **Serena ledger** — read **`.serena/memories/ledger.md` directly** (Read tool). The full
   chronological history of what was done and why. Newest on top.
   ⚠️ **Serena gotcha:** do NOT use `read_memory("ledger")` via the Serena MCP tool — it returns one
   giant single-line JSON (`{"result":"…"}`, ~150 KB) that overflows the read limit. Read the `.md`
   file directly, or `jq -r '.result' <savedfile>` on the file the tool saves.
3. **`reference/RECONSTRUCTION.md`** — the 60-class work-list + live status table.

If those three and this file agree, you're oriented. If they disagree, the
ledger + git log are ground truth; fix the docs.

**Reference docs (the recovered contracts + coverage maps — read the relevant one before
touching that area):**
- `reference/ENGINE_INTERFACE.md` — the COMPLETE engine↔mod contract (every DLL_FUNCTIONS
  callback + cldll export + protocol), each marked faithful/partial/stock/missing. **Start
  here for "what's left" — it lists the real interface gaps.**
- `reference/API_SURFACE.md` — every cvar (104)/command (92)/message (74)/event (11) from
  the binaries + a R/read/★wired implementation-status table (§6c).
- `reference/HUD_MESSAGES.md` — the user-message wire formats (the protocol contract).
- `reference/HUD_DRAWCHAIN.md` — the HUD render path recovered from client.dll assembly.
- `reference/VERIFICATION.md` — the playtest checklist + the (unmapped) silencer findings
  + the think-behaviour status.
- `reference/coverage/REPORT.md` — **drop-in interface coverage** (original vs ours): the
  by-name swap contract — entity classes, cldll exports, console commands, server+client cvars
  (w/ default + wire-size mismatch), user messages, events, entity keyvalues — matched/missing/
  extra. Missing = RE/implement work-list; extra = invented or HL-leftover to remove. **THE
  roadmap** — see the "Coverage harness + RE flow" section below for how to use + regenerate it.
- `reference/coverage/RECONSTRUCTION_COVERAGE.md` — per-TS-class **method-completeness** metric
  (140/368 = 38%, least-complete first: CTSStunt 0/25, CTSSharedEmitter 0/11, …). `scripts/coverage/reconstruction.py`.
- `reference/coverage/VTABLES.md` — every original class's **vtable layout** (slot→method, all
  226 classes), so a decompiled `call [vptr+0xNN]` resolves to the method. `scripts/coverage/extract_vtables.py`.
- `reference/WEAPON_SELECT.md` — the weapon-select protocol (invnext/invprev cursor → `weapon_<N>`
  + holster→deploy; the real command set vs our stand-ins).
- `reference/BUY.md` — the buy/loadout-config command protocol (`tkw`/`tki`/`tka`/`rst_cfg`/`cashala…`).
- `reference/EVENTS.md` — **the 11 TS `.sc` weapon events** (fire/reload/draw/grenade/knife/katana/
  coldcock/closecombat/pump/empty/firelinked): the complete RE map (server handles + client
  hook→handler addrs + playback sites) + the build order for the client weapon-param subsystem.
- `reference/BULLETS.md` (cont.69u) — the **bullet/breakable/slow-mo/sound** RE: live fire path
  `ShootBulletsPlayer` + penetration, glass shatter (`Die__10CBreakable` → gmsgTSCreateDebris sprite
  shower vs TE_BREAKMODEL), dive-through-glass (`CheckForBreakables`), CBulletManager slow-mo bullets,
  knife→glass (`WaitThink` swept-break), + the sound-pitch TODO. Read before touching weapons/breakables.

## ✅ DONE (cont.69x) — the TS client PARTICLE / impact-effect subsystem is reconstructed + LIVE-VERIFIED
The cont.69w next-task is **complete**. The original's custom client particle pool (NOT engine tempents,
NOT the old R_SparkShower) is byte-ported in **`cl_dll/ts_particles.{h,cpp}`**, hooked into
`HUD_DrawTransparentTriangles` (`cl_dll/tri.cpp` → `TSParticles_Update()`), and routed from
`cl_dll/ev_ts.cpp`: `EV_TSBulletImpact` → `TS_ImpactEffect` (per-surface dispatcher), `EV_TSMelee` →
`TS_MeleeImpact` (glint + sparks only). Full RE map: **`scratchpad/impact-effects-re.md`** (on-disk,
gitignored). What's in it, all verified by **live gdb traces of the running original** (not guessed):
- **Crux render/update** = `FUN_100520d0(pool)` (the handoff's stated blocker) — located via xrefs to the
  pool object `DAT_1018bde8` from `HUD_DrawTransparentTriangles`. Per-frame: integrate `pos += vel*frametime*
  _DAT_10079e88(10)`, gravity `vel.z -= dt*gravity*0.1*sv_gravity(800)=×80`, drag, brightness fade
  (`-=dt*fade*100`), size accel, frame anim, TriAPI billboard draw (`gEngfuncs.pTriAPI`).
- Manager (`FUN_1004ffe0`/`100509d0`/`100509a0`/`10050b60`), billboard (`FUN_10050ce0`/`10050080`, camera-
  facing + velocity-STRETCH), 5 spawners (smoke `46680` / debris `46bd0` / scintille `46f50` / spark-shower
  `492b0` / pellets `46a70`), dispatcher `FUN_10048f60` (per-surface char switch + 33/50/70 RandomLong gating).
- **Smoke = a conic flare**: 4 puffs marching out `pos=origin+normal*size*0.8*i` (size ×0.8/step), each
  velocity-STRETCHED (stretch 7) + growing (accel 0.1); **velocity divisor DECREMENTS** `/(count-i)` so tip
  puffs eject faster; frameRate `20±10` jitter. (Spawn args + every motion field byte-matched live.)
- Bullet decals = **server-side** `DecalGunshot` (entry + exit) so all clients see them; resolves the TS
  `{shot` from `ts/decals.wad` (the Valve `{shot` asterisk only appears if that wad isn't resolved). The
  glass-crack/`{bproof` decal on a non-breaking glass hit is handled by `CBreakable::DamageDecal` on this path.

### ⚠ Known approximations / deviations (faithful BEHAVIOUR, not byte-exact rendering)
- **Billboard velocity-stretch geometry** (`FUN_10050080`) is reconstructed, not byte-ported corner-for-corner.
- **Debris tumble**: implemented as a screen-space roll (angle += spin*dt*10, spin=RandomLong(-180,180)); the
  original rolls via the engine billboard fn `DAT_100d5b88` = `hw.dll`. Visual, close enough.
- **Concrete debris** renders `kRenderTransAlpha(4)` not the RE's `TransColor(1)` — our TriAPI draws INDEXALPHA
  TransColor as an unmasked square; (4) masks it to the flake. Engine-quirk workaround.

## ✅ DONE (cont.69y) — kung-fu KICKS dispatch is now RE-faithful (CCombatAttack__6CTSGun @0x86078)
Rewrote `CTSGun::CloseCombatAttack` (dlls/ts_gun.cpp) as a faithful port of the decompiled dispatch
(scratchpad/ccombatattack.c). Decoded the untyped pev offsets: button@0x180, flags byte@0x1a5 (FL_ONGROUND
0x200 / FL_DUCKING 0x4000), iuser4@0x250 (lock mask 0x50c30); pulled every DAT_ constant from the binary
(damage 35/45/55). Kung-fu is FULL-AUTO; refire = `local_108` (normal punch **0.25s** = 4/sec cap, uppercut
0.65, basic kick 0.3, finisher 0.5, heavy kicks 0.75–1.35). **NOTE:** my first pass read refire as `local_10c`
(0.1s) — WRONG: Ghidra mis-attributed the load (the `CCombatAttack` tail mis-symbolizes as `SlowMotionFlow`,
boundary-confused frame analysis shifted the slot 4 bytes). The user live-confirmed the original = ~4/sec
full-auto (hold == click) = 0.25 = `local_108` (DAT_00107e98) EXACTLY; `local_10c` is the victim block-stun.
KICK selection now mirrors the original: **duckkick** (onGround+duck) → **back-kick**
(moving-backward: +back key when slow, vel·facing≤−0.5 when fast) → **dropkick** (airborne+speed>270+duck+
moving-forward) → **3-kick ground combo → spin finisher** (the `m_kickCombo`/this+0xa00 counter, no-repeat
cycle 0xa/0xe/0xd/0xf) → **flying kick** (fast+airborne, cycle 0x13/0x12/0xf). PUNCH side also made faithful:
persistent `m_comboStep`/0x9fc counter (3 jabs → uppercut; reset only on finisher, NO time-window), air-punch
when airborne, uppercut on combo>2 OR a duck-tap; all punches 35 dmg. Builds clean (hl.so + client.so, glibc
2.14); **USER playtested + confirmed "this looks good"** (full-auto 4/sec, visible 3-jab→uppercut). Also
recovered the canonical attack-id→body-anim names from `HandleCloseCombat` (0x10=sidekick, 0x14=duckkick, …).
SLICEs (documented in-code): the forward whiff-LUNGE (orig
local_110 → player+0x79c, a decaying PreThink velocity push on a miss) is a separate PreThink subsystem not
yet ported; the iuser4 lock-state (0x50c30) and player+0x814/0x815 state bits (special-punch 0x17 / forced
finisher) default to "clear". Original ids 0xa..0x19 remapped to v_melee.mdl's 11 seqs (dispatch/dmg/timing
byte-faithful, anim a remap).

## ⭐ NEXT TASK — the remaining WEAPON-FEEL TODOs (see the 🔴 list below)
Kung-fu kicks done; open work carried from cont.69v: dynamic-spread edge cases, weapon sway, coldcock whip,
breakable respawn, katana throw. Use the same **live-trace-don't-guess** method (`scripts/live.py`).

### Task-tool entries — ALL COMPLETE (cont.69x)
#1 locate update/render fn · #2 struct/pool · #3 manager · #4 update/render+hook · #5 spawners (RE-audited
from raw ASM) · #6 dispatcher+wrapper · #7 route events / drop R_SparkShower · #8 glass-crack decal — all ✅.

### ✅ COMPLETED + DEPLOYED this session (cont.69w) — see the ledger for detail
- **Angle-recoil** reconstructed (cl_dll/view.cpp TS_AddViewRecoil/TS_UpdateViewRecoil): per-shot aim climb,
  ease `frametime*15` (cap 1), persistent, snap <0.1°. **The cont.69v "0x648c0/vtable[0x38]" lead was WRONG**
  (that's a vgui Panel getAbsExtents). Real source: FUN_10008230 (client twin of server VKickback) → recoil
  TARGET accumulator → eased in V_CalcRefdef (FUN_10057ed0). Guns = climb only (NO shake).
- **Melee/kung-fu "bounce-back" shake** (the screen-shake — melee only, guns don't shake): knife/katana
  amp=flDamage*0.15, kung-fu amp=min(power,45)*0.35 (punch 20/kick 25), dur 0.2/freq 10, transient render-
  only. view.cpp TS_TriggerViewShake/TS_ApplyMeleeShake; triggered in CTSGun::MeleeAttack/CloseCombatAttack.
- **Bullet penetration** (dlls/ts_gun.cpp TSFireBulletsPenetrate): rewrote to the original ShootBulletsPlayer
  (@nm 0x5f69c) uniform PointContents loop — penDepth=min(dmg*2,48); pass-through gated by world-solid
  thickness (so depends on weapon dmg + enter/exit angle). Removed the special glass branch.
- **Glass** no longer always breaks: PointContents ignores entity brushes so bullets pass through glass;
  it shatters only if dmg >= pane health (=20 in TS maps). **cont.69x FIX:** per-pierce damage_adjust =
  the `damagemult` cvar value (default 0, RE-matched to ShootBulletsPlayer — `*(damagemult+0xc)`), NOT 0.75
  — so two glass panes break only the first and DECAL the second (was breaking both at 0.75).
- **Weapon table now byte-exact** (dlls/ts_weapon_info.cpp): the Write_<weapon> values are an EARLIER stage
  OVERWRITTEN by Sparta()/SetSpartaWeapon (server) == client FUN_10007a80 (identical). Added SetSparta()
  + 32 calls (spread/viewkick/recoil/damage); fixed m61/knife/katana/sealknife/flag Write_ outliers; added
  Write_flag (idx 29). Verified 0 mismatches vs binary Write_ AND Sparta. See [[sparta-overrides-write-weapon]].

### 🔴 STILL-OPEN weapon-feel TODOs (carried from cont.69v, not done)
- ✅ **Kung-fu KICKS dispatch** — DONE (cont.69y), RE-faithful port of CCombatAttack__6CTSGun (above).
- ⏳ **Dynamic spread edge cases**: m60 5× recoil (field_0xc8==0x20), barrett max-spread-moving (==0x12).
- ⏳ **Weapon sway**: amplitude smoothing + per-weapon grip + forward term not ported (figure-8 IS done).
- ⏳ **Coldcock viewmodel whip** (coldcock.sc LAB_10043800); **breakable periodic respawn**; **Katana
  right-click = CTSGun::KatanaThrow**.

### 🔴 LIVE-original debugging — scripted: `scripts/live.py` (+ CLAUDE.md §4b, `live-original-ts-debuggable` memory)
The user **keeps the original TS running in `hl.exe` (Proton/Wine), ALWAYS available to gdb-attach.** When
static RE dead-ends on a virtual/indirect/client-side path, **live-trace it — don't guess** (guessing = the
cardinal sin). **`scripts/live.py`** codifies the workflow (built cont.69v):
`live.py pid` / `base <mp.dll|client.dll>` / `resolve 0xADDR…` (→ module+RVA+nearest export) / `bt` (pipe a
gdb backtrace, resolves every frame) / `read <module> 0xRVA [f|d|u|p]` / `readabs 0xADDR` / **`vmethod 0xOBJ
<slot>`** (this→vtable[slot] → resolve) / **`watch <module|abs> 0xRVA|0xADDR --go "msg"`** and **`break …
--go`** — run these BACKGROUNDED: they print `>>> ARMED — <msg>` once set (tell the user the action), block
until the trip, then print the resolved backtrace; a value-`watch` fires only on CHANGE (no mouse → only the
recoil moves `cl.viewangles`). **The recoil step is now ~3 commands:** `break client.dll 0x648c0` (read
`param_1`/the object from the regs+stack dump) → `vmethod <param_1> 14` (→ the `vtable[0x38]` recoil fn) →
decompile + reimplement → `watch abs 0x2d8aec4 --go "fire, no mouse"` to verify the kick matches the original.

### In-flight TODO (no formal task-tool entries this session — tracked here; newest weapon-feel work)
- 🥇 **Angle recoil** — located (above); reconstruct `vtable[0x38]`'s math. THE next task.
- ⏳ **Kung-fu KICKS (right-click) not RE-faithful** — the FIRE punch combo IS done (`field_0x9fc`: 3 jabs →
  uppercut). The kick side still uses our OLD condition approximation, not the RE'd `field_0xa00` 3-kicks+
  finisher counter + the back/drop/side/spin/air-kick dispatch. Full RE table is in CCombatAttack @0x86078
  (decompile via `scripts/ghidra/re.sh 0x96078`; the cont.69v ledger has the decoded move table). Model seqs
  are authoritative for anim indices (v_melee.mdl: punch_left2 punch_right3 uppercut7 kick5 sidekick6 …).
- ⏳ **Dynamic spread — edge cases skipped**: `field_0xc8==0x20` (m60 = 5× recoil), `==0x12` (barrett =
  max-spread-when-moving). The core (ScaleSpread grow + PreThink decay) IS wired.
- ⏳ **Weapon sway — simplified vs the RE**: amplitude SMOOTHING (state DAT_10191d6c/d78), per-weapon GRIP
  scaling (0.1/0.3/0.5/0.7/1.0 by anim-ext), and the forward base-position term `forward·(0.75−amp·0.5)`.
  The figure-8 pendulum + mid-air stop ARE done (constants read live from client.dll FUN_10066e70).
- ⏳ **Kung-fu wall smoke** is a server `TE_SPRITE` approximation; the original likely spawns it client-side
  via the impact dispatcher (same path as bullets) — RE the kung-fu client event if you want exact fidelity.
- ⏳ **Knife slash viewmodel cycle** uses deterministic `m_slashStep` (verified seq indices: knife {3,4,5},
  katana/sealknife {2,3,4}); the original cycles 3 body anims + the knife.sc client event — fine, but VERIFY.
- ⏳ Older queue (still open): **Coldcock viewmodel whip** (RE coldcock.sc LAB_10043800), **breakable periodic
  respawn** (`BreakableRespawn`/`RespawnThink`/CBreakable::Die respawn branch), **Katana right-click** =
  `CTSGun::KatanaThrow`.

### Big tooling win this session — `scripts/ghidra/re.sh` (server-RE orchestrator) + ApplyInventory
`re.sh setup` (fresh project + ApplyInventory: member/vtable structs, typed engine API, FP consts, clean
names) then `re.sh <Class>` / `re.sh 0x<ghidraAddr>` / `re.sh refs <Class>` decompiles read like source.
**The `re.sh` output filter was fixed cont.69v** (it was dropping multi-line decompile bodies — now full
typed bodies print). See CLAUDE.md §4a.1. (re.sh is server-only / `ts_i386.so`; for **client.dll / mp.dll**
decompile-by-address use **`scripts/ghidra/dc.sh <client|mp|server> 0x<rva>`** — built cont.69v; feed it the
RVA straight from `live.py resolve`/`bt`.) `extract_weapons.py` is UNRELIABLE (stack-offset collisions corrupt values) — the cont.69v
weapon-table was re-verified by decompiling all 38 `Write_<weapon>` functions; trust those, not the heuristic.

**Method reminders:** RE-ground every value (the user REVERTS guessed work). ⚠️ FILE OFFSET for `.rodata`
constants = **Ghidra addr − 0x10000** (the FP pool reads 0 at the naive offset; many consts are DOUBLES the
decompiler shows as `(float)`). Tooling: `scripts/ghidra/{DecompAt,DecompRefs,FindStr}.java` (server proj
`recovered/ghidra/server` name `serverso`, client `recovered/ghidra/client` `clientdll`, Windows
`recovered/ghidra/mpdll` `mpdll`); Ghidra addr = `nm` + 0x10000 for the server `.so`. `scratchpad/` is
gitignored (decompiles/RE notes live there). Commits are `--no-gpg-sign` (gpg passphrase expires mid-session);
work on `main`; no `Co-Authored-By` trailers. Don't commit the user's `test.sh` (their launcher).

## Where things stand (2026-06-24, through cont.69w) — cont.69w (recoil/shake/penetration/glass/weapon-table)
## was BUILT + DEPLOYED to the xash rig AND COMMITTED on `main` as **72e96fa** (`--no-gpg-sign`). Still
## uncommitted on disk: `test.sh` (the user's launcher — DO NOT commit) + pre-existing `reference/coverage/*`
## edits (not cont.69w work). Older state below is through cont.69t:

### cont.69t — weapon-effect polish batch (RE-grounded; user playtests). Read the ledger top for full detail.
- **Bullet impact smoke** (8430ea3) — client `EV_TSBulletImpact` (cl_dll/ev_ts.cpp): a SMOKE puff per surface
  material at the hit (RE: client dispatcher `FUN_10048f60` — wood/dirt→wood_smoke_add, metal/glass/grate/vent/
  computer→gun_smoke_add+R_SparkShower, concrete/default→gun_smoke, flesh→none). Server already did damage+
  texture-sound+decal+blood; this adds the visual. Reuses `PM_FindTextureType` + `EV_TraceTexture`.
- **knife.sc melee spark** (6876988) — `MeleeAttack` plays `events/knife.sc` on a non-flesh surface hit →
  client `EV_TSMelee` R_SparkShower at the impact (origin carries the point).
- **Predicted melee/grenade anims** (62d754e) — knife/katana slash (SendWeaponAnim slash1 = v_knife 3 /
  v_katana 2) + swing/hit sounds; grenade throw seq. (Once CTSGun is predicted, these stick.)
- **Knife right-click THROW** (dee455c + c16e960) — `dlls/ts_knife.cpp` = `CTSKnife` projectile. SecondaryAttack
  (fire-type KNIFE) → `CTSGun::ThrowKnife` (spends m_iClip, throw seq v_knife 7 / v_sealknife 5, auto-switch when
  dry). COLLISION (RE: BounceTouch@0x6decc): STICK in non-breakable surfaces (embed), BOUNCE off unbreakable
  breakables, BREAK-THROUGH breakables it can kill + stick in the wall beyond (ignore_glass forward trace),
  stab flesh. Pickup = press-E (IN_USE) in a trigger radius (NOT run-over). **Fixed a server crash** (Host_Error
  "trigger in clipping list" — relink after changing pev->solid).
- **Dropped-weapon GLOW** = WHITE `kRenderFxGlowShell` (CTSWorldGun + stuck knife) — model shell, not a light.
- **Breakable shatter** — ALREADY WORKS (stock HL CBreakable::Die TE_BREAKMODEL by material; verified glass→
  glassgibs on ts_kissmeyerbasic). No change needed.

### cont.69s — ✅ CTSGun is CLIENT-PREDICTED; the `.sc` weapon-prediction subsystem is COMPLETE. Read `reference/EVENTS.md`.
- Steps 1–2: shared `s_weapon_info` table into client (`dlls/ts_weapon_info.{h,cpp}` built into both DLLs) +
  the active-weapon param cache (`cl_dll/ts_weapon_predict.{h,cpp}`, `g_tsActive`) + the draw event.
- **3a–3d: CTSGun is now a real CLIENT-PREDICTED weapon** so its viewmodel anims STICK. `ts_gun.cpp` compiles
  into client.so (class extracted to `dlls/ts_gun.h`; client stubs in `cl_dll/ts_gun_stubs.cpp`); `g_TSGun`
  registered in `cl_dll/hl/hl_weapons.cpp` (HUD_InitClientWeapons + the TSGUN_WEAPON_ID case); state shipped
  via `CTSGun::GetWeaponData/SetWeaponData` (type/reserve/firemode). **The key fix = TIME BASE:** CTSGun's
  timers reverted from absolute `gpGlobals->time` to `UTIL_WeaponTimeBase()` (34 sites) + `UseDecrement()`
  returns true, so the engine/server decrement loop drives the 0-base correctly. Fire/akimbo events use
  FEV_NOTHOST (no double muzzle). Verified predicted (listenserver) AND server-only (dedicated rig, bots).

## Where things stood (2026-06-23, through cont.69r) — all committed (git log: 1ca0ca4 / c675ef0 / 7a66e5c)

### cont.69q–r — kung-fu close-combat + many playtest fixes (all RE-grounded, in-engine/user-verified, COMMITTED)
- **Akimbo OWNERSHIP refactor** (items[]-by-type) — see cont.69p note below; done + verified.
- **Firing fixed** (was fire-once-stuck): CLIENT_WEAPONS makes `UTIL_WeaponTimeBase()`==0, but CTSGun is
  server-authoritative → all its timers now use `gpGlobals->time`. **Loadout fixed:** batched `tkw/tki/tka`
  parse ALL argv (client sends one space-separated cmd); `BuyArmament` no longer resets so the config
  persists across spawns/round-restarts. **Deploy fixed:** `TSDeployArsenal` forces the tsgun active +
  `CanDeploy()` override (TS uses its own ammo). **Round-restart weapon-loss fixed:** `PlayerSpawn`
  `RemoveAllItems` before re-equip (was making a 2nd tsgun → arsenal vanished).
- **Kung-fu** (TSW_MELEE = bare hands → `CCombatAttack`): contextual dispatch (FIRE = punches only —
  uppercut on duck, combo on hold, single punch on tap; +ATTACK2 = kicks only — sidekick/spinkick/backkick/
  kick); reload = `CCombatBlock` "hooah" (player/block.wav) + stamina → breathe2.wav; flesh/surface/miss
  sounds (kungfuhit / closecombat+pl_step / closecombat); v_melee anims remapped to the model's 11 seqs;
  3rd-person BODY anims via `CTSAnimation::PlayKungFu` (the player models carry punchleft/sidekick/… by
  name) + `combat_idle` stance; no phantom 3rd-person held weapon (empty weaponmodel when no p_model).
- **Muzzle flares**: `ts_muzzleflash1h/6` sprite at the muzzle attachment (akimbo per-barrel). **Shotgun**:
  per-shell incremental reload (interruptible by firing). **`thirdperson`** is now a toggle.
- **`.sc` events**: complete RE map + subsystem plan committed (the NEXT TASK above).

### ✅ DONE (cont.69p) — the akimbo OWNERSHIP refactor is IMPLEMENTED + in-engine verified.

### ✅ DONE (cont.69p) — the akimbo OWNERSHIP refactor is IMPLEMENTED + in-engine verified.
`CTSGun`'s arsenal is now the original's **items[]-by-tsweapon_t model** (`struct TSItem m_items[TSW_COUNT]`
in dlls/ts_gun.cpp = {owned, clip, leftClip, rightClip, mode, attach, barrelMask}). A single can be
wielded/dropped independently of its akimbo pair. RE in `reference/WEAPON_SELECT.md` (TakeWeapon@0x80a04 /
CTSGun::DropWeapon@0x8094c / CBasePlayer::DropWeapon@0x84900 / SwitchItems=FUN_00081564 /
GetBetterWeapon@0x80f40 / g_iWeaponPriority). Combine keeps BOTH items[single]+items[akimbo] owned (scroll
skips the single, slot1-5 still picks it); dropping a single while owning its akimbo throws one + keeps the
other (clip = akimbo.rightClip) + removes the akimbo; dropping the akimbo throws two singles + removes both;
useitems cycles the active barrel mask; GetBetterWeapon auto-selects by priority. Verified in xash3d
(combine via `tsbuy 2`×2, slot-select-single-while-owning-akimbo, drop-single-keeps-other; no crashes).
**Plus three cont.69p playtest fixes (see ledger):** (1) the spawn→spectate "crash" was a launch-flag
issue — the listenserver needs **`+deathmatch 1 +maxplayers 8`** (else stock respawn() does
`SERVER_COMMAND("reload")`); CLAUDE.md §3 updated. (2) kung-fu no longer drawn red in the select bar
(clipless weapons can't be "out of ammo"). (3) dropped weapons no longer block the player while falling
(RE: original gives the falling world gun a ZERO-size bbox; ts_worldgun.cpp falling SetSize → g_vecZero).
**NOT yet committed.** Remaining: exhaustive 38-weapon in-engine retest.

### Recent playtest fixes (cont.69m–n) — all RE-grounded + in-engine/user-confirmed
- **Weapon-select REBUILT** (`cl_dll/ts_weaponbar.cpp` + `ammo.cpp`): the stock yellow `CHudAmmo` menu is
  retired; the TS bar IS the select UI (RE: FUN_1004f500/1004f220/1004d2cf/1004e030). slot1-5 + invnext/
  invprev open it + move a cursor; **FIRE commits** `weapon_<N>` (server holster→deploy, plays
  weapons/switch.wav); closes on commit/cancel (no timer). Bar ALWAYS draws but very transparent; selecting
  brightens it, cursor most opaque (3-level additive opacity, NO box); cursor shows curclip/maxclip.
- **Death = spectate** (`dlls/player.cpp PlayerDeathThink`): on death, after the anim, re-enter the free-look
  spectate/buy phase (TSEnterSpectate); respawn on FIRE after the gate (NOT a timer). Verified via `kill`.
- **Spectator chrome suppressed** in the TS phase (`tri.cpp` overview grid; `vgui_SpectatorPanel.cpp`
  #Spec_Duck). The dark bottom bar uses `pfnFillRGBABlend` (pfnFillRGBA is ADDITIVE → black was invisible),
  geometry = the stock bottom border (YRES_HD(64), label at YRES_HD(6)).
- **Buy menu**: first open per map randomizes the loadout ("Use New Config"); later opens reload the last
  config ("Use Last Config"); Open() guards on already-open (the server resends the spectate msg each tick —
  was causing a reopen-flicker after apply). **TSMessage SVC notification** client handler added
  (`cl_dll/ts_message.{cpp,h}`, CHudTSMessage — fixes the "No pfn TSMessage" error); "Using new/last config"
  + "Press fire to play" route through it (centred, fades out). Kung-fu given on every spawn. Status panel
  hidden while dead/spectating.

### ✅ THE REAL BUY MENU — REBUILT as a HUD element (cont.69l). cl_dll/ts_buymenu.{cpp,h}.
The real cascade buy menu (the `0x10037xxx` cluster) is **rebuilt + in-engine verified**: a `CHudTSBuyMenu :
CHudBase` that draws the header + cascade columns (Config|Category|Weapons|Detail) with `TSHud_DrawString`
+ the TS HUD font, navigated by **number keys (HUD_Key_Event) AND mouse (IN_MouseEvent — user-confirmed
working)**. The legacy vgui `vgui_TSBuyMenu.*` is **deleted**. Full RE in `reference/MENUS.md` "★★ THE REAL
BUY MENU" (rendering, input state machine FUN_10038840, mouse FUN_10038360/10038410, accounting, apply,
the weapon cost table). **Verified vs the user's reference screenshots**: real roster (Beretta 92F #5 +
Contender G2 #10 in Handguns), "Special Purpose"/"Go Back", green "7 Random", blue Cancel, the 1-5 loadout
preview columns (bought items by HUD column, in add-order, count-left-of-icon, `(Sil ,Las ,Fla ,Sco )`),
`Free Slots = 81 - used` (remaining), Total/slots accounting all match.
**Auto-ammo DONE (runtime-captured).** The menu auto-fills each weapon's caliber to max + adds the cost to
Total + shows the rounds left of the preview icon (Glock 210 / M16 120) + emits `tka <cal>_<units>`. The
per-caliber **max** field reads 0 statically (load-time only), so the caliber table `DAT_1009ffc0` + per-weapon
caliber (`weapon_info+0x08`) were read from the LIVE hl.exe (`/proc/<pid>/mem`, client.dll base 0x0c070000,
table @0x0c10ffc0) — full table in `reference/MENUS.md` "AUTO-AMMO". Verified Glock → 350cr ammo → Total 3400.

**Spectate HUD fixed (cont.69l):** TS has NO stock spectator chrome — the inherited HL/TFC `SpectatorPanel`
top "Map:"/timer/score bar was bleeding through. `SpectatorPanel::Update()` now suppresses it (top+bottom
borders + Map/timer/scores) during the TS spectate phase, and `CHudSpectate::Draw()` draws the dark bottom
bar + "Free-Look" + the control help in the HUD layer (no longer hidden under the vgui overlay).
Screenshot-verified vs the original (no top bar; bottom bar matches).

### (historical) THE BUY MENU course-correction
**Read `reference/MENUS.md` "★ THE REAL BUY MENU" — the rebuild spec.** The TS client.dll has
**two** buy-menu code paths: the **legacy/dead** one (`0x10010xxx`, a **vgui::Panel**, literal strings, the
`DAT_1007a950` table) — which is what `cl_dll/vgui_TSBuyMenu.{cpp,h}` currently reproduces (cont.69e–j:
config + category + weapon-list + per-weapon attachment submenu + mouse/keys + compact green styling, all
built + in-engine verified) — and the **REAL** one the game actually draws (`0x10037xxx`). The user's
reference screenshots (cascade columns, "Special Purpose", "Go Back", attachment costs, Beretta 92F +
Contender G2 in Handguns) are the REAL menu. All on-disk client.dll copies incl. the Steam install are
byte-identical (688128b) — **same binary, we just RE'd the wrong builder.** **User decision: RE + rebuild
the real cascade menu.** RE is COMPLETE (cont.69k); the **rebuild is the next task**.

**The real menu (full RE in MENUS.md — the rebuild target is a `CHudBase` HUD element, NOT vgui):**
- **HUD-drawn** — entries via `FUN_1004ff10` → **`FUN_10021ce0` = `TSHud_DrawString`** + the TS HUD font
  `DAT_100d5908` + `titles.txt` token localization. So it reuses our existing HUD draw/font (it'll look right).
- **Cascade**: orchestrator `FUN_10036750` (switch on `obj+4` = level 0 config/1 category/2 weapons/3 detail/
  4-7 accessory&c/8 off); columns side-by-side at X=8/0x88/0x108/0x188, rowH 0x14, width 0x80; top header bar
  (`FUN_10036af0`: Total + Free Slots). Builders: config `FUN_10037540`, category `FUN_100377d0`, weapons
  `FUN_10037960`, detail `FUN_10037ba0`.
- **Real weapon roster** (`obj+0x2d6c`, indexed `row*5+cat`, filled by `FUN_10036200`): Handguns
  [1,9,12,14,2,21,22,28,31,36], SMGs [3,6,7,17,19,23], Rifles [5,13,15,27,18], Shotguns [4,11,20,26,33],
  Special Purpose [24,25,32,34,35]. **Attachment costs**: Silencer 900, Lasersight 800, Flashlight 500,
  Scope 1800. Colours: available=half-hud dim, over-budget=red(195,45,45), Random/affordable-Buy=green
  (45,195,45), Cancel/GoBack=blue(100,100,205).
- **Still to RE for the rebuild**: the input/level-advance handler, the accessory column (states 4-7), the
  `FUN_10038180` weapon-preview geometry. The legacy-menu RE findings on per-weapon attachment data
  (0xE0/0xE4 masks, akimbo 0xC8, full 0xDC) + the styling (compact font, colours) CARRY OVER.
- **Also raised by the user (separate, not started):** the weapon-select bar (invnext/invprev) doubles as the
  loadout preview (bought items before applying config) AND the in-game weapon HUD when ALIVE, showing
  per-weapon attachment icons — `CHudTSWeaponBar` (Image #4 in the consult thread).

### Where things stood (through cont.69f)
Pipeline fully working: decompile → recover typed prototypes → write modern C++ →
**build into `hl.so`/`client.so`** → **run in-engine** (ReHLDS for server; xash3d-fwgs
for the graphical client — this box HAS a GPU) → **gdb** any crash. Everything below is
committed (`git log`); `recovered/` is gitignored.

### cont.69 (this batch) — kevlar gameplay + the TS buy menu. Read `reference/MENUS.md` first for the menu.
- **DONE + verified:** `powerup`/`cashala` cheat cmds (server cmds 0 missing); the **TS status panel**
  (posture figure + health♥ + Kevlar) replacing the stock HEV (`cl_dll/ts_status.cpp`); **stock HEV/battery
  suppressed**; **kevlar GAMEPLAY** — `CBasePlayer::TakeArmor` (binary flag + armorvalue) + the `armor_absorb`
  flat % damage reduction in `TakeDamage` (NOT a depleting pool; bullets-only, melee/fall/blast bypass via
  mask 0x80040e4) + `AddItem` + buy routing (`dlls/ts_buy.cpp`, `dlls/player.cpp`, `dlls/ts_gamerules.h`
  `m_flArmorAbsorb`). All RE-grounded, in-engine verified.
- **TS BUY MENU — IN PROGRESS (reproduction, `cl_dll/vgui_TSBuyMenu.{cpp,h}`).** Mechanism FULLY RE'd
  (`reference/MENUS.md`): it's a **vgui::Panel of vgui::Button** entries (NOT MOTD/gmsgVGUIMenu, NOT
  gmsgShowMenu), nav by mouse-click OR bound number key, apply emits `rst_cfg`/`tkw`/`tki`/`tka`. Built +
  committed: **Config + Buy-category + Player Accessory pages** (RE'd from FUN_10010440/@0x10010dd0) + the
  **number-key routing**. **NOT done:** (1) per-category **weapon lists** — builders identified
  (`FUN_10010890`/`FUN_100108b0`/`FUN_10010bb0`, the action-`0xb` thunks — see MENUS.md); (2) the config
  panel's **exact colours/positions** — provably not statically RE-able (the `+0x2d84` config panel has no
  alloc/store/sizer in the disasm); resolved via a **runtime hbreak capture of the live original** (TS runs
  under Proton; SOFTWARE breakpoints don't fire in wow64 but HARDWARE `hbreak` do; the menu is built ONCE and
  cached, so arm the hbreak BEFORE a clean relaunch+first-open — exact procedure in MENUS.md). Placeholder
  values applied (dims 128×20, white config text) are clearly flagged, not guessed.
- **HARD RULE reaffirmed by the user this batch (RE before code, no guessing):** every menu entry/colour/
  position/command must come from the decompiled builder or the runtime capture — do NOT improvise styling
  or layout. The user reverted earlier guessed work (orange→green→white text, made-up geometry, a top-level
  "Buy Kevlar") until it was RE-grounded.

### ⚠️ Serena ledger read gotcha
`read_memory("ledger")` returns ONE giant single-line JSON (`{"result":"…"}`, ~150 KB) that overflows the
read limit. **Don't read it via the MCP tool** — read `.serena/memories/ledger.md` directly (newest on top),
or `jq -r '.result' <savedfile> | head -c 9000` on the file the tool saves.

(Original cont.68l baseline pipeline note retained below.)

> ### ⚠️ READ FIRST — hard requirement (cont.66, user-set): **RE before you code.**
> On any user feedback, **check the decompilation FIRST** — do not improvise. Every
> **console command** the client sends is part of the swap contract, so verify it exists
> in the original before using it (server cmds = `ts_i386.so` ClientCommand dispatch;
> client cmds = `client.dll` `pfnAddCommand`; `strings`/`signatures-client.txt`). The
> original uses **standard inputs** (`+use`/`IN_USE`, `+attack`, `invnext`/`invprev`,
> `+alt1/2`) + server-side **Think** logic — NOT custom `ts*` commands. `menuselect` +
> `gmsgShowMenu` ARE the real buy-menu mechanism. See the memory `check-re-on-feedback`.

### ⭐ Coverage harness + Ghidra RE flow (cont.67–68 — the way to continue)
The drop-in goal is now **measured + driven by a harness**, and the Ghidra decompiles are
**typed + de-truncated**. This is the loop a new agent should use:

**1. Find the next gap — the coverage harness (`scripts/coverage/`, pure objdump/nm/readelf, no Ghidra dep):**
```bash
python3 scripts/coverage/coverage.py all     # -> reference/coverage/{original,ours}.json + REPORT.md
python3 scripts/coverage/reconstruction.py   # -> reference/coverage/RECONSTRUCTION_COVERAGE.md (method %)
python3 scripts/coverage/extract_vtables.py  # -> reference/coverage/{vtables_original.json, VTABLES.md}
```
`REPORT.md` diffs the EXTERNAL swap contract (the only thing that must match for drop-in — NOT the
internal C++ ABI, per CLAUDE.md): entity classes, cldll exports, server+client console commands,
server+client cvars (w/ default + message wire-size mismatch), user messages, events, entity
keyvalues, + a `default.cfg` input cross-check. **MISSING = implement/RE; EXTRA = invented or
HL-leftover to remove.** Cvars/usermsg-sizes come from the BINARY (pfnRegisterVariable / cvar_t
structs / LinkUserMessages), NOT config.cfg (config.cfg holds only the archived subset — that bit us).

**2. RE the gap — the Ghidra flow (persistent, typed):**

**Server side — prefer the `re.sh` orchestrator** (one entry point; applied project at `recovered/ghidra/re`):
```bash
scripts/ghidra/re.sh setup            # run.py (refresh inventory) + fresh import/analyze + ApplyInventory.
                                      #   Do once, and after coverage/script changes. FRESH each run (rename
                                      #   step isn't idempotent). ApplyInventory ⊃ ApplyTypes: builds member +
                                      #   vtable structs, types entvars_t/enginefuncs_t/FP consts, clean names.
scripts/ghidra/re.sh <Class>          # decompile every method of a class (addrs via filemap.py = nm + 0x10000)
scripts/ghidra/re.sh 0x<addr> ...     # decompile by address
scripts/ghidra/re.sh refs <Class|addr># who CALLS/uses it (DecompRefs over all the class's methods)
scripts/ghidra/re.sh export <Class>   # -> recovered/decomp/<Class>.c
scripts/ghidra/re.sh members          # DumpMembers -> reference/coverage/members_auto.tsv (transient members:
                                      #   offset+type, placeholder names — curate into members_re.tsv)
```
Reads now show `this->m_flExpire`, `this->vtbl->Precache(this)`, `g_engfuncs->pfnSetModel(...)`, `dc_0_1`.
See CLAUDE.md §4a.1 for what ApplyInventory/DumpMembers/DecompRefs do + the correctness guards.

**Client / mp.dll / lower-level** — `setup-ghidra.sh` builds the persistent `recovered/ghidra/{server,client}`
projects (DisableNoReturn + ApplyTypes), and you can still drive a single decompile by hand:
```bash
analyzeHeadless recovered/ghidra/client clientdll -process client.dll -noanalysis \
  -scriptPath scripts/ghidra -postScript DecompAt.java 0x<ghidraAddr>
python3 scripts/coverage/vtcall.py CTSGun 0x128      # resolve a vtable-dispatch offset -> method
python3 scripts/coverage/vtcall.py --file recovered/server/CTSGun.c CTSGun   # annotate a decompile's vt calls
```
Ghidra scripts (`scripts/ghidra/`): **ApplyInventory** (the omni applier; superset of ApplyTypes — see §4a.1),
**DumpMembers** (transient-member offset+type skeleton), **DisableNoReturn** (preScript — stops the noreturn
truncation), **FixNoReturn** (postScript), **ApplyTypes** (gcc2 prototypes + `entvars_t` + MSVC returns),
**DecompAt** (decompile by addr, auto-creates a function if Ghidra missed it), **DecompRefs** (callers; takes
multiple addrs), **DecompExport** (decompile a whole class to a `.c`), **FindStr** (string search).
NOTE: `ClientCommand` is a huge function Ghidra still truncates — for its command dispatch use **objdump**
disassembly + resolve the PIC string operands via the GOT (`_GLOBAL_OFFSET_TABLE_` = 0x165E10): the pattern
is `lea -0xoff(%ebx) ; … ; call <strcmp@plt>` → string at `0x165E10 - off`. The relocation table
(`readelf -W -r`) is the master key — it resolves vtables, cvar_t name/default pointers, and message-name
pointers that all read as ZERO in the file (they're R_386_32/RELATIVE relocs filled at load).

**3. Implement + verify the loop closes:** write modern C++ → `scripts/build.sh amd64 [all]` → re-run
`coverage.py all` (the gap should move from missing/extra → matched) → run in xash for visual items
(screenshot; this box has a GPU) → commit. **Commands done this way (cont.67–68): weapon-select
(`weapon_<N>`/`lastinv`), `firemode`/`drop`/`useitems`, the buy cmds (`tkw`/`tki`/`tka`/`rst_cfg`/
`cutwire_`), `respawn`/`motd`/`jointeam`/`changeteam`, + the WAITING scoreboard, + the `powerup`/`cashala` cheats
(cont.69).** Server console-command coverage: 12 missing → **0**; 0 unanswered default.cfg binds.

### Track B cont.66 — TS join / spectate / buy-config / weapon-select flow (IN PROGRESS)
The faithful join flow (no straight-to-play spawn) is built + verified; the buy menu and
weapon-select are partly done with **known unfaithful stand-ins to fix**. Plan file:
`~/.claude/plans/replicated-discovering-aho.md`.

**DONE + verified (committed `0d4f2d0`/`60a8c44`/`d1b0974`/`a1a8149`):**
- **Spectate-gate (server):** a joining/respawning player is held in **free-look observer**
  (`CBasePlayer::TSEnterSpectate` → `StartObserver`/`OBS_ROAMING`, respawn gate =
  `respawntime` cvar clamp 3..30; `dlls/player.cpp`). `CTSGameRules::PlayerSpawn` skips the
  loadout while `m_fInSpectateWait`; `CBasePlayer::Spawn` ends by entering spectate;
  `ClientPutInServer` sets the flag pre-Spawn. **Press fire → `TSLeaveSpectate` → respawn
  into play** with the chosen loadout. gmsg `Spectator`(2)/`VGUIMenu`(1) registered.
- **Free-look HUD (client, `cl_dll/ts_spectate.{cpp,h}`):** centre countdown → "Press fire
  to play" + the free-look help line. Reads `Spectator`; hooked via `__MsgFunc_Spectator`
  repointed to it.
- **Buy/config menu — the FAITHFUL mechanism:** compact **`gmsgShowMenu` + `menuselect`**
  list (NOT a VGUI panel — RE'd from `client.dll FUN_10011160`: item 1 toggles Use New/Last/
  Empty Config, available items green). `TSSendBuyMenu`/`TSMenuSelect`/`TSCloseMenu` in
  `dlls/player.cpp`; client `CHudMenu` draws it (`\g` green added in `cl_dll/menu.cpp`).
  "Click anywhere" (+attack while menu up) = item 1. The `buy` command reopens it
  (spectate-only).
- **Weapon-bar layout (client, `cl_dll/ts_weaponbar.cpp`):** RE'd from `FUN_1004e030` —
  5 columns `x=col*0x80-0x78`, header panel + **slot number** (alpha 100 active/50 idle),
  weapons stacked 0x30/row with sprite icons (red when clip empty), active weapon name+ammo
  to the right. Verified vs the user's reference.

**KNOWN-UNFAITHFUL / OPEN (fix RE-first — see the ⚠️ above):**
- ✅ **Weapon switching — DONE (cont.67, RE-grounded + in-engine verified;
  `reference/WEAPON_SELECT.md`).** Removed the invented `tsnextweapon`/`tsprevweapon`.
  Faithful: `invnext`/`invprev` (client `CHudTSWeaponBar::CycleType`) step through the owned
  arsenal and commit via the original client's transport **`weapon_<N>`** (numeric
  `tsweapon_t`, from `FUN_1004f500`'s `hud_fastswitch` commit); server `weapon_<N>`
  ClientCommand → `TSSelectWeaponType` → `CTSGun::SelectSlot` with **holster→deploy**;
  `lastinv` toggles to the previous weapon. DEFERRED: the `hud_fastswitch 0` open-bar +
  on-bar highlight + literal (row,col) grid cursor (we commit immediately + walk the compact
  owned-set).
- ✅ **Command names — DONE (cont.67–68, RE-grounded, coverage-verified):** `firemode`/`drop`/
  `useitems` (renamed from tsfiremode/dropweapon; `useitems`→akimbo swap), the buy/config cmds
  `tkw`/`tki`/`tka`/`rst_cfg`/`cutwire_<n>` (renamed from tsarm/tsarmammo/tsresetarmament/tscutwire —
  arg formats RE'd, `reference/BUY.md`), `respawn`/`motd`/`jointeam`/`changeteam`. `usepowerup` IS a
  real cmd (kept). Server console-command coverage 12 missing → **2** (`powerup`, `cashala` remain);
  0 unanswered default.cfg binds. `tsbuy`/`tsbuyammo`/`tsbuyarmament`/`tsspawnflag` remain dev
  stand-ins (no 1:1 original — the original buys via queue tkw/tki/tka → apply `cashala` config).
- ✅ **Scoreboard "WAITING" — DONE (cont.68, verified):** spectate-waiting players show "WAITING"
  in the scoreboard class column (DM + teamplay), playerclass 11. The per-team **locked model** is set
  by `AssignToTeam`. OPEN: confirm our inherited `ClientUserInfoChanged` enforces the model-lock the
  original does (resets a changed `model` infokey — see cont.68k ledger).
- **OPEN — `powerup` + `cashala` commands** (need RE): `powerup` (distinct from `usepowerup`); `cashala…`
  = the buy config-apply (parses `weapon_%i`/`lasclr_%i` — finishes `reference/BUY.md`).
- **OPEN — timed-powerup activation**: still on the `usepowerup` command path; the original ALSO has the
  double-tap `+use`(`IN_USE`)-in-PreThink path (RE `UsePowerUp`/PreThink) — reconcile.
- **Not yet built:** bar **faint-when-idle fade** (`FUN_1004d3f0` select-active); active
  **clip/maxclip "8/8"**; **powerup-timer HUD** (Slow Motion / Double Fire Rate text, green,
  above the weapons, from `PwUpTime`); **kung-fu / super-jump icons** (centre-bottom);
  buy-menu **cursor + click-on-entries**; the **"2 Buy" weapon-category submenu**;
  menu **background box**.
- **Open question (cont.67):** `buy` IS real — original `ts/default.cfg` binds `b`→`buy`
  (also `firemode`/`useitems`/`drop`/`usepowerup`/`invnext`/`invprev`); the menu is
  server-driven (`gmsgShowMenu`). cont.66's `buy` reopen is faithful. The precise dead/alive
  availability gate is a TODO for the coverage harness + a targeted `buy`-handler RE. See
  `reference/WEAPON_SELECT.md`.

**Target = INTERFACE/protocol compatibility, so each half is a drop-in swap for the
original** (our client ↔ original server, or vice-versa, behaves identically). What must
match the original byte-faithfully: the engine C interface + the client↔server protocol
(user-message **names + wire formats**, **event** names/params, **command** names, **cvar**
names). What must NOT be chased: the internal C++ ABI / member offsets / gcc-2.x quirks — we
build on the newer Itanium-ABI `halflife-updated` SDK. See CLAUDE.md "compatibility target".

**Big picture: the entire server side is reconstructed (~40 TS classes); Track A is COMPLETE and
Track B (the client) has its build foundation.** Done + verified: the full weapon engine, powerups
(dynamic spawn + slow-motion bubble), the death loop (weapons/ammo/corpse + grenade shrapnel), the
**complete team subsystem** — team assignment (cont.35), cash + free buy/armament (cont.36, the
`CashBased()`-is-off finding), all three round-end conditions (elimination cont.37, objective cont.31,
**timeout** cont.45) — **every CTF/objective/escape entity** (`CTSFlag`/`CTSBriefcase`/`CTSBomb`/
`CTSSellZone`/`CTSTeamEscape`/`CTSScoreGiver`/`CVipEscape`, cont.38–43), and the map/relay entities.
Exercised on real maps: tm_avana, ctb_deadheat, ts_dust2, ts_theatre, ts_wyndham.
**What remains is Track B — the cl_dll (client):** the client BUILDS (both 32- and 64-bit, cont.46) and
the **first TS client HUD has landed — the round-time countdown `CHudRoundTime` (cont.47, `cl_dll/
roundtime.{cpp,h}`)**, the first reader of a server user message. Still to do: the *visible* slow-motion
time-dilation, the particle system, the stunts, and the rest of the HUDs (the server already sends the
user messages; the client just needs the readers). The client now **runs + renders in xash3d-fwgs on
this box** (it has a GPU — the old "headless" note was wrong; see Track B below for the screenshot loop).
**Watch the gmsg NAMES:** the inherited server named
the round-time message `"TSRoundTime"` but the original `.so`/client.dll use `"RoundTime"` (fixed in
cont.47) — check every other inherited gmsg name (`TSPowerUpTime`/`TSMotionBlur`/team-info) against the
`.so` strings before writing its client reader (`strings reference/ts_i386.so | grep -E '^TS|^Round'`).
Small server leftovers: `CTSPlayerAmount`/`CTSSuperJump` done (cont.44); `CInfoBreakable` (thin) +
`CBulletManager` (bullet-time — best done alongside Track B) remain. See "Recommended next steps".

### Track B playtest reconstruction — IN PROGRESS (cont.47–50)
The client runs in xash3d-fwgs and the user is playtesting it; each report exposes that the stock HL
client + a partial weapon/player reconstruction is showing through. Fixed so far (all decomp-grounded):
**round-time HUD** (cont.47), **client weapon-prediction crash** (cont.48, NULL `AllocString`),
**spawn loadout** (cont.49, `weapon_tsgun`+`BuyArmament`, no HL weapons), **always-mouselook** (cont.49,
fixed the "can't look up/down + jolt"), **fire animations** (cont.49, `SendWeaponAnim` from per-weapon
`shoot*` model sequences), **akimbo fire anims** (cont.50, right/left barrel), **HUD sprite-res fallback**
(cont.50), **reload** (cont.50, `CTSGun::ItemPostFrame` drives the per-caliber reserve — the stock base
gated reload on standard ammo TS never uses).

**The user asked for a WHOLESALE faithful reconstruction of the player/weapon/input/HUD systems from the
decomp (not symptom-patching). The 5-phase plan (tracked as tasks):**
1. **Weapon firing/reload/ammo/fire-rate** (CTSGun) — reload ✅; TODO fire-rate gate, `SecondaryAttack`/
   `+alt1`, what networks to the ammo HUD.
2. **`CBasePlayer` thinks/state + the low-pitch "slow-mo" fire sound** — likely the slow-motion time-scale
   (TS lowers sound pitch in slow-mo); decompile `PreThink`/`PostThink`/`TSUpdate` + the slow fields.
3. **All TS console commands / cvars / input toggles** (`+alt1`, `+reload`, `hud_color`, `cl_realhud`, …)
   enumerated from the `.so` and wired server+client (fixes "Unknown command" spam + missing keys).
4. **The TS client HUD** (the reason there is NO HUD at all): TS ships a *fully custom* HUD — `hud.txt` is
   320/640-only, no stock crosshair/ammo/health path. Reconstruct from `client.dll` + the `gmsg` readers
   (`TSHealth`/`TSArmor`/`TSCash`/`TSState`/`TSMBlur`/`TSSlowMo`/`TSBTime`/`TSPSys`/`RoundTime`).
   **IN PROGRESS (cont.51): health/armor/cash DONE + visually verified** — the full TS user-message wire
   contract is recovered in **`reference/HUD_MESSAGES.md`**; server sends `TSHealth`/`TSArmor`/`TSCash`
   from `CBasePlayer::TSUpdateClientData` (`dlls/ts_player.cpp`); client `CHudTSStatus`
   (`cl_dll/ts_status.{cpp,h}`) draws them bottom-left. Also fixed 3 inherited protocol-name/size bugs:
   `RoundTime` (-1→size 2, 2 BYTES not 2 shorts), `TSMotionBlur`→`TSMBlur`, `TSPowerUpTime`→`PwUpTime`.
   **NEXT: the ammo HUD** (`WeaponInfo`(6)/`WStatus`(3)) — needs the per-arsenal-slot struct (@gun+0xcc,
   0x70/slot) decoded to pin the bytes; CTSGun never sends `ClipInfo`, so clip rides inside WeaponInfo.
   Then `TSState`/`TSSlowMo`/`TSBTime`/powerup HUDs, and suppress the leftover stock HUD strip.
5. **Client view (bob/sway) + predicted fire events** — viewmodel "snap" (`cl_bob 3` + stock `V_CalcBob`;
   TS bob math differs) and moving fire to the faithful predicted `events/fire.sc` (`EV_TSFire`).

- **Map entities — runtime-verified:** `CTSModel` (108 on `ts_kissmeyerbasic`),
  `CTSMapGlobals` (weapon-restriction on `ts_trinityfall`), `CTSGroundWeapon`
  (`ts_wyndham` pickups), `CTSPowerUp` (22 on `ts_theatre`, bots pick up),
  `CTSGroundAmmo` (`ts_groundammo` refill pack, 6 on `ts_supertimor_d3`),
  `CTSSlowMotion` (`ts_slowmotion` zones, 4 on `ts_wyndham`). Built + synthetic/factory-only
  (no stock map hits them): `CTSFog`, `CTSTimer`, `CTSCustomTarget`, `CTSDMHill` (cont.32),
  `CTSTrigger` (cont.34), `CTSSlowMotionPoint`/`CTSShrapnel`/`CTSWorldGun`/`CTSAmmoPack`/
  `CTSDeadCorpse`/`CTSFlag` (cont.38, the CTF flag — carry/capture/drop) (runtime-created, not
  map-placed). **`CTSBriefcase`** (cont.39, `ts_briefcase`) is map-placed — verified on the real CTB
  map `ctb_deadheat` (2 briefcases each seed their team's CTSFlag at base + capture there).
- **Objective entities (team-mode maps) — runtime-verified:** `dlls/ts_objectives.cpp` —
  `CTSObjectiveManager`/`CTSObjectivePointer`/`CTSHack`/`CTSWinGiver`/`CTSTrigger` (+ the
  `CTSObjective` base). All spawn on `tm_avana` (0 Can't init, was 7) + `tm_corporate`; CTSHack
  counts `hackingtime` down to completion (fire target+HUD); CTSWinGiver Use declares the
  `winnerteam` → **`CTSGameRules::TeamWin` scores + restarts the round (cont.31)**; CTSTrigger
  fires its target on a valid team-gated touch (cont.34) — now driven by real `pev->team` (cont.35).
  **SLICE:** TeamWin cash + `ShowObjectiveOnHud` + the client objective HUD pointer are deferred.
- **Game rules:** `CTSGameRules : CHalfLifeMultiplay` IS the live `g_pGameRules`
  (`ts_gamerules.{h,cpp}` + `InstallGameRules`). Real TS overrides now: `GetRandomPowerup`,
  `Think` (dynamic powerups), `PlayerKilled` (death loot + `CallCheckDeads`), `TeamWin` +
  round-restart, `CheckDeads`/`CallCheckDeads` (cont.37 — team-round elimination win condition),
  `GlobalSlowMotion2`/`SetGlobalSlowMotion`, `PutPowerUp`. **Team subsystem (cont.35,
  `dlls/ts_teams.cpp`):** `IsTeamMode`/`IsTeamplay` (team mode iff a `ts_objective_manager` is
  present — the same DM proxy cont.26 used), `InitTeams`/`RecountTeams`/`GetTeamIndex`/
  `GetIndexedTeamName`/`GetTeamID`/`TeamWithFewestPlayers`, `AssignToTeam`, and a `PlayerSpawn`
  override that **auto-balances** any unassigned player onto the fewest team (server-side stand-in
  for the faithful client class menu). `pev->team` is **0-based** (0 spec, 1 merc) to match the
  FGD/objective gating. **Verified:** 5 bots balanced 3/2 on `tm_avana`; DM assigns nothing.
  Cash + free buy/armament are built (cont.36, `dlls/ts_buy.cpp` — `CashBased()` is off so weapons are
  free; `tsbuy`/`tsarm`/`tsbuyarmament` console-drive it, the menu UI is cl_dll); the round timer is
  built (cont.45, `StartRoundTimer`/`RoundTimeLeft` → draw on timeout). Still inheriting MP / deferred:
  the faithful `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), the full
  `RestartRound`@0x67920, and friendly-fire. `TSGameRules()` accessor + `TS_PWUP_*` bits.
- **Player:** no `CTSPlayer` — TS state lives on `CBasePlayer` (`player.h` member
  block `m_iPowerups`/`m_iHeldPowerup`/`m_flPowerupTime`/`m_flPowerupExpireTime`/
  `m_flHealthRegen`/`m_iKungFu`/`m_bLowGravity`) + accessors in `ts_player.{h,cpp}`.
  `TSHudMessageAll`/`TSHudMessageOne` in `ts_utils`.
- **Powerups — runtime-verified (slice):** `CTSPowerUp` (`ts_powerup.cpp`, entity
  `ts_powerup`, 328 placements) — full lifecycle (TOSS-drop → settle → touch-pickup
  → respawn); spawnflags = the random-powerup **mask** (not a single type). Picks a
  random type from `GetRandomPowerup`, applies it via free func `PlayerAddPowerUp`.
  **Instant powerups real** (kungfu/lowgrav/health/armor/grenade/inf-clip), **timed
  ones held** (`m_iHeldPowerup` + duration). **SLICE:** armor/inf-clip/grenade use
  functional substitutes (no `TakeArmor`/`MultiplyClips`/`PickUpWeapon`);
  `gmsgTSPowerUp` HUD + lowgrav physics-key deferred (client.dll side).
  **Dynamic spawning done (cont.26):** `CTSGameRules::PutPowerUp` + a `Think` override spawn
  a random powerup at a DM spawn every ~20s (DM maps only).
- **Powerup activation — runtime-verified (slice):** `CBasePlayer::UsePowerUp`
  (`usepowerup` console command) + `UpdatePowerUpState` (per-frame in `PreThink`) +
  `DeactivateSlowMotionFlow`, all in `ts_player.cpp`. A held timed powerup activates
  → `m_iPowerups` set (so `SlowMotion()`/`DoubleFireRate()` accessors flip + the slow
  fields `m_flSlowMotionScale`/`Ramp`/`m_bSlowBullets` are applied each frame) →
  expires after its duration → restores. `gmsgTSPowerUpTime`/`gmsgTSMotionBlur`
  registered + sent.
- **Slow-motion state + zones — runtime-verified (slice):** `CBasePlayer::GoSlow`/
  `InSlow` (`ts_player.cpp`; the TS slow-motion virtuals, as plain player methods —
  `GoSlow` sets `m_flSlowMotionGo`/`Target`+`m_bSlowBullets`, `InSlow` reports it),
  `CTSGameRules::GlobalSlowMotion2`/`SetGlobalSlowMotion` (`ts_gamerules.cpp`; slow
  all players, throttled ~0.5s), and the **`CTSSlowMotion`** brush zone (`ts_slowmotion`,
  11 placements, `ts_slowmotion.cpp`) — touch-slows players, tracks + drops them on
  leave, "All map" mode → `GlobalSlowMotion2`. Verified on `ts_wyndham`: zones spawn,
  `Use()` activates, `GlobalSlowMotion2(0.5)` flips `InSlow()`/`go=0.5`, restore → 1.0.
- **Powerup→slow propagation DONE (cont.33):** `CTSSlowMotionPoint` (`dlls/ts_slowmotionpoint.cpp`)
  is the slow-motion bubble — an activated slow-motion/slow-pause powerup spawns one at the player
  (`UsePowerUp`), and it `GoSlow`s every non-owner player with line of sight for the duration, then
  restores them. (Un-SLICEs the old "activated slowmo sets `m_flSlowMotionScale` but doesn't `GoSlow`".)
- **SLICE — the visible time-dilation layer is still deferred (client-entangled):** the
  actual slow-motion *movement* (the client pmove reads the slow fields — no cl_dll
  reconstructed); `GlobalSlowMotion2` slowing projectiles (needs `GoSlow`/`InSlow` as
  **CBaseEntity** virtuals — currently CBasePlayer-only); `InterpolateSlowMotion`; the
  client motion-blur HUD; the on-death powerup re-drop. `CTSSlowMotion`'s exact Spawn
  trigger-setup + `m_flRadius` are inferred (// VERIFY).
- **Weapons — runtime-verified, arsenal + all 38 types + fire-type dispatch (slice):**
  `CTSGun` (`ts_gun.cpp`, entity `weapon_tsgun`, id 16) holds a multi-weapon
  **arsenal** (`m_arsenalType/Clip/Mode[]` + active slot — a modern stand-in for the
  original `items[]`/`m_pActiveItem`; **ammo is per-CALIBER, `m_ammoReserve[16]`, shared**).
  `AddWeapon`/`SelectSlot`; `GiveTSWeapon` adds to the held gun; the `tsnextweapon` command
  cycles; per-slot clip/mode persist across switches; ground-weapon pickups stack into the
  arsenal. `PrimaryAttack` → fire-mode gate → `Fire()` dispatches by **fire-type** with
  recovered per-weapon rate/spread: **semi** (one/press), **full-auto** (held), **shotgun**
  (8 pellets), **melee** (knife 48u/katana 64u hull-trace, no bullets). Full `s_weapon_info`
  table in `ts_weapons.{h,cpp}` (incl. `flFireRate`/`flSpread`/`flRecoil`/`iCaliber`/`iPellets`/
  `iFireModes`). Roster precached at map load via `CWorld::Precache`.
  Powerup hooks wired: inf-clip `MultiplyClips`, double-firerate halves `flFireRate`.
  **Grenade fire-type (2) throws an M61** (`CM61Grenade`, `ts_grenade.cpp`); its blast now
  throws a **`CTSShrapnel`** fragment burst (cont.29).
  **Akimbo fire-types (4/5) dual-wield** (cont.21): picking up a 2nd of a single pistol you
  already hold upgrades that arsenal slot in place to its akimbo variant (doubled clip),
  via the recovered `s_weapon_info` +0xc8/+0xcc pairing; `AkimboFire` fires full-auto-feel
  alternating the muzzle side (±5u `v_right`).
  **Fire-mode system** (cont.22): a per-slot fire-mode (auto/semi/burst/pump) gates the
  trigger, cycled by `SwitchFireMode`/the `tsfiremode` command over the recovered
  `s_weapon_info+0xc` mode mask; `PrimaryAttack` → `Fire()` → `StandardFire`/`AkimboFire`;
  burst = 3 rounds (2 for the MAC-10), driven by `WeaponIdle`; defaults per `TakeWeapon`.
  **Per-caliber ammo** (cont.23): reserves are pooled by the weapon's `s_weapon_info+0x8`
  caliber (`m_ammoReserve[16]`, shared across same-caliber weapons); `AddAmmo`/Reload draw
  rounds capped at the `compile_ammo_info` max table; CTSGroundAmmo refills per caliber via
  `TSAddAmmo`.
  **Shotguns** (cont.24): fire the real **8 pellets** (`iPellets` = `s_weapon_info+0x24`);
  mossberg/sawedoff fire-types fixed (1 pump / 8 semi).
  **Dropped weapons** (cont.27, `dlls/ts_worldgun.cpp`): `CTSWorldGun` + the `dropweapon`
  command — drop the active weapon into the world (`RemoveActiveWeapon`), it falls/settles,
  another player presses **USE** to pick it up (`TSPickUpWeapon`).
- **Death handling — runtime-verified (cont.28, 30):** `CTSGameRules::PlayerKilled` override
  (`ts_gamerules.cpp`) drops the dead player's arsenal as `CTSWorldGun`s + a `CTSAmmoPack`
  (`dlls/ts_ammopack.cpp`, the per-caliber reserve, consumable) + spawns a fading `CTSDeadCorpse`
  (`dlls/ts_corpse.cpp`, the frozen studio pose, fades over 15s). Verified: kill a bot → 1 corpse
  + its weapons + an ammo pack drop (USE to grab guns; walk over the pack). SLICE: live-pose copy,
  no gib/blood, melee not dropped.
- **Objective win → round restart (cont.31):** `CTSGameRules::TeamWin` (`ts_gamerules.cpp`) — a
  `ts_wingiver` completing now scores the team + announces + restarts the round (respawn all +
  re-arm objectives) after 3s. SLICE: no cash, no round-timer HUD, no per-player team assignment.
- **King-of-the-hill (cont.32):** `CTSDMHill` (`dlls/ts_dmhill.cpp`, `ts_dmhill`) — hold the
  brush volume alone `nextcheck`s → `fragamt` frags + announce. Built + synthetic-verified (0
  stock placements; a hand-made map would confirm in-map).
  **SLICE limits:** akimbo is a single active clip (not a true left/right barrel split) with
  no IN_ATTACK2 "fire both barrels at once"; no buy menu (start reserve = clip*3); no
  client-side fire-mode/ammo HUD.
- **WATCH OUT:** `recovered/` decompiles can be **truncated** by Ghidra noreturn
  artifacts (cut `CTSModel::Spawn` at `UTIL_SetSize`, `CTSPowerUp::Spawn`/`OnGround`
  at `UTIL_SetOrigin` — the cut tail held a `nextthink` set + the `SetTouch`/`SetThink`
  arming). Cross-check raw `objdump -d` at the symbol's real `.so` addr (Ghidra adds
  0x10000 vs `nm`) when a decompile looks too short. Runtime crashes → **gdb the
  backtrace**; a "spawns but nothing happens" symptom → check for a missing
  `pev->nextthink` (think never fires).

## The loop (commands that work on this box)
```bash
cd ~/Projects/goldsrc.net/mod-ts
# decompile one class (recovered/ is gitignored; output annotated w/ prototypes)
scripts/decompile.sh reference/ts_i386.so recovered/server CTSCustomTarget
# exact prototypes for any function:
grep 'CTSCustomTarget::' reference/signatures-gcc2.txt
# entity keyvalues/spawnflags:
grep -A12 '= ts_customtarget ' reference/ts_fgd.fgd
# write dlls/<file>.cpp, add it to dlls/CMakeLists.txt, then build + verify:
scripts/build.sh amd64
nm -D build-amd64/dlls/hl.so | grep ' ts_customtarget$'      # factory exported
nm build-amd64/dlls/hl.so | c++filt | grep 'CTSCustomTarget::'
```
Tools are on PATH: `Ghidra`/`analyzeHeadless` (12.1.2, `~/Tools/ghidra`), `rz-*`
(`~/Tools/rizin`), `scripts/gnv2demangle.py`. The buildchain image
`hlds64-buildchain:debian10` is already built. Ghidra/rizin/objdump read the
**original** `ts_i386.so`; **`gdb` debugs our own `hl.so`** — when a runtime test
segfaults, `gdb -batch -ex run -ex bt --args ./hlds_linux …` for the backtrace
instead of guessing (it found the `EV_Precache`/`DeathNotice` crashes). See
CLAUDE.md §4. For code needing a live **player** (weapons/powerups), the rig has a
working 64-bit **metamod+rcbot** bot setup (procedure + boot/gdb commands in
`reference/RECONSTRUCTION.md` "In-engine test rig"). For bulk **data tables** from
the `.so`, `scripts/extract_weapons.py` is the pattern (PIC string + numeric
extraction → C table).

## Recommended next steps
**The whole server side is done — Track A is COMPLETE (cont.35–45).** The clear next focus is
**Track B (the client/cl_dll)**, whose build foundation just landed (cont.46). Track A below is kept
for reference (all ✅) + its remaining *fidelity polish* (optional, low value). Read Track B first.

**→ Track B — the cl_dll (IN PROGRESS, now RUN + VISUALLY verifiable on this box).** The client builds
(`build.sh <arch> client`, 32- and 64-bit) AND **runs + renders in native xash3d-fwgs** — the cont.47
**`CHudRoundTime`** HUD is **visually confirmed** (a yellow `1:51` M:SS clock, top-centre, cont.48).

**IMPORTANT — this box is NOT headless** (the cont.46 caveat was wrong): it has an AMD GPU + X display
(`DISPLAY=:0`). So mod-ts client work is **self-verifiable here with a screenshot** — no need to wait on
the user. The loop:
```bash
scripts/build.sh amd64 client          # (and amd64 for the server)
scripts/setup-xash-mod.sh              # assemble the native xash game root (re-run after every rebuild)
cd ~/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64
LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana +sv_lan 1 +maxplayers 4 &
# wait for "client connected" in the log, then:  import -window root /tmp/shot.png
# crop the game window's top-centre (window @ 1280,180, 2560x1080):
#   convert /tmp/shot.png -crop 1000x220+2060+180 +repage /tmp/clock.png
```
gdb the client the same way as hl.so when it crashes (it found the cont.48 CGlock/AllocString crash):
`LD_LIBRARY_PATH=. gdb -batch -nx -ex run -ex bt --args ./xash3d -game ts +map tm_avana`.

**→ The 5-phase reconstruction — STATUS THROUGH cont.64** (full chronological detail in the Serena ledger
cont.51–64; the recovered contracts are in `reference/ENGINE_INTERFACE.md` / `API_SURFACE.md` /
`HUD_MESSAGES.md` / `HUD_DRAWCHAIN.md` / `VERIFICATION.md`).
- **Phase 4 — TS client HUD ✅ DONE + playtest-verified (cont.51–53).** Three regions from `client.dll`
  assembly: **weapon-select bar** (`CHudTSWeaponBar`, real `sprites/weapons/*.spr` icons, fed by `WStatus`),
  **ammo box** (`CHudTSAmmo` ← `WeaponInfo`), **health bar** (`TSHud_HealthBar`). Exact geometry/colour/
  alpha/font; HUD colour `hud_r/g/b`=150/175/255; per-char clipped `TSHud_DrawString` (= `FUN_10021ce0`).
  Polish left: attachment rows (need the WeaponInfo flags byte + silencer), active-highlight, left block.
  **Status panel ✅ (cont.69b, `cl_dll/ts_status.{cpp,h}`):** the bottom-left **TS status panel** that
  replaces the stock HEV health/battery — team name + a **posture figure** (`sprites/player/{stand,crouched,
  prone}.spr`, RE'd from the @0x10049d1f sprite-load block) + **health number + heart** (`health.spr`) +
  a binary **"Kevlar"** indicator (`kevlar.spr`, drawn only when `m_iArmor>0`; armor is kevlar-or-not, NOT
  a number — the old "Armor: N" was wrong). Crouch live via local `curstate.usehull==1`; **prone is
  CTSStunt-gated** (dormant `g_iTSProne` hook). Screenshot-verified stand/crouch/100+heart/Kevlar. OPEN:
  the buy/loadout doesn't yet grant kevlar armor (server gap); prone posture awaits CTSStunt.
- **Phase 3 — commands/cvars ✅ DONE (cont.54).** ~25 client + ~25 server cvars registered with exact
  defaults; commands registered. No "Unknown command" spam. Several cvars now **wired to gameplay**:
  `roundtime`, `slowmatch` (→ global slow-motion match), `damagemult` (→ weapon damage), `initial_cash`
  (→ starting cash, cash HUD). Status table: `API_SURFACE.md` §6c.
- **Phase 5 — predicted fire event 🟡 PARTIAL (cont.56, fixed cont.64).** Firing uses `events/fire.sc` →
  `EV_TSFire` (`cl_dll/ev_ts.cpp`) for the **fire sound + brass shell + viewmodel muzzleflash**; the
  **viewmodel "shoot" anim is server-driven via `SendWeaponAnim`** (cont.64 — the event's EV_WeaponAnimation
  doesn't stick since CTSGun isn't client-predicted). True client prediction (CTSGun in HUD_PostRunCmd) TODO.
- **Phase 2 — low-pitch fire sound ✅ INVESTIGATED, no defect (cont.55).** Sound path correct; re-test.
- **Phase 1 — weapon firing/reload/ammo ✅.** `+alt2` coldcock done (cont.57/64). TODO: fire-rate gate,
  `+alt1`/silencer (UNMAPPED — `VERIFICATION.md` §B), akimbo sound via the event.

**Other (cont.57–64):** `+alt2` coldcock pistol-whip; **thirdperson fixed** (MP gate removed + camera
behind the player, cont.61/64); `GetGameDescription` → "The Specialists (mode)"; **VISIBLE slow-motion
movement** (PreThink scales speed cap by the slow factor, cont.62 — the marquee effect).

**NEXT FRONTIER (all gameplay/subsystem, see `ENGINE_INTERFACE.md` "notable gaps"):**
1. ✅ **`CTSAnimation` — DONE (cont.65, `dlls/ts_animation.{h,cpp}`).** The player 3rd-person gait
   (idle/walk/run/crouch/jump/fire/reload by grip) is reconstructed + **visually verified** (the always-idle
   bug is fixed). The original `SetAnimation` is a no-op stub; TS drives the body via `CTSAnimation::Animate`
   (now an embedded `m_TSAnim` member, driven from PostThink/fire/reload/death). pev->sequence = torso
   (`idle_/aim_/shoot_/relo_/draw_<grip>` + crouch_/duck_ variants, jump, die_spin, grenade); pev->gaitsequence
   = legs (run2/walk2/crawl). **Deferred (CTSStunt-gated, never fires until CTSStunt lands):** dives/cartwheels/
   walljump/morfy/rolls (activities 0x28-0x32,0x36-0x39); also TODO: directional-aim strafe blend
   (BlendAnimationsByDir, 8-way), death-by-direction variants (we use die_spin), confirm the fire/aim torso
   pose (hard from the behind-camera — needs a front view or rcbot).
2. **Full pmove time-dilation** — slow-motion movement is wired (speed cap); gravity/jump-arc/anim-rate
   time-dilation needs the slow value in client-predicted `pm_shared`.
3. **`CTSStunt`** (dives/rolls/wall-flips, 25 funcs, PostThink — unlocks the deferred CTSAnimation branches) +
   **true client weapon prediction** (HUD_PostRunCmd) + the **`+alt1` silencer** (unmapped) + the **buy
   menu** (cl_dll VGUI).
4. **rcbot-in-xash rig** would make combat-gated things testable (damagemult, coldcock damage, fire path).

**Workflow:** build → `setup-xash-mod.sh` → run in xash (the loop above) → screenshot/gdb to verify →
commit → update docs. Every message/event/command/cvar name must be **byte-faithful to the `.so`** (the
swap-compatibility contract — see "Where things stand"); check `strings reference/ts_i386.so` /
`reference/client.dll` before naming anything.

**Pattern for a TS HUD element (cont.47, follow `cl_dll/roundtime.{cpp,h}` + the stock `battery.cpp`):**
(1) confirm the server's REG_USER_MSG string matches the original `.so` (`strings reference/ts_i386.so`);
(2) `class CHud<X> : CHudBase` with `Init`/`VidInit`/`Draw`/`Reset`/`MsgFunc_<Msg>`; `DECLARE_MESSAGE` +
`HOOK_MESSAGE(<Msg>)` in `Init` + `gHUD.AddHudElem(this)`; (3) read with `BEGIN_READ`/`READ_SHORT`;
(4) wire `#include` + `CHud<X> m_<X>;` into `hud.h`, `m_<X>.Init()`/`.VidInit()` into `hud.cpp`, and the
`.cpp` into `cl_dll/CMakeLists.txt`; (5) `build.sh amd64 client` + `i386 client`, `nm|c++filt` for the
methods, `strings` for the hook name.

---

**Track A — the `CTSGameRules` team subsystem + cash (✅ COMPLETE, cont.35–45).** Kept for reference;
all steps done. Remaining is fidelity polish only (UpdateTeamScores/InitTeamScore, the full faithful
RestartRound, ShowObjectiveOnHud, CheckDeads grace/CTB/last-man branches) — diminishing returns.
1. ✅ **Team assignment (DONE, cont.35)** — `dlls/ts_teams.cpp`: `pev->team` set 0-based via
   auto-balance on spawn (`PlayerSpawn` → `TeamWithFewestPlayers`); the team roster + GetTeamIndex/
   GetTeamID mirror `CHalfLifeTeamplay`. Verified: bots get balanced teams. The CTF/escape entities
   (step 4) are now unblocked. (Faithful join/class menu is cl_dll — deferred.)
2. ✅ **Cash + buy/armament (DONE, cont.36)** — `dlls/ts_buy.cpp`. **Major finding: `CashBased()` is
   hardcoded OFF in the .so**, so weapons are FREE — the buy system is a free armament (loadout)
   selector, not an economy. `GiveCash`/cash member kept (cash-for-win/lose, future sell zone) but
   never charged. `ResetArmament`/`AddItemToArmament`/`AddAmmoToArmament`/`BuyWeapon`/`BuyAmmo`/
   `BuyArmament` reuse `GiveTSWeapon`/`TSAddAmmo`; M61 is team-mode-only + capped 2/life. Driven by
   console commands (`tsbuy`/`tsarm`/`tsbuyarmament`/…) since the menu UI is cl_dll. Verified on
   tm_avana + DM. **Remaining buy SLICE:** the cl_dll buy menu, the category weapon restriction
   (`IsRestrictedWeapon` over CTSMapGlobals — default 0x1f allows all), starting/win cash, and the
   dead price field (`s_weapon_info+0xd8`, only live if a cash economy is ever turned on).
3. **Team scores + the full round** — ✅ elimination (cont.37) + ✅ the **round timer** (cont.45):
   `CheckDeads`/`CallCheckDeads` end a round when a team is wiped out; `StartRoundTimer`/`RoundTimeLeft`
   time the round out (default 120s) → fire `targetontimeleft` + declare a draw. Rounds now end three
   ways (elimination / objective / timeout), all via `TeamWin` + restart. **Still TODO:** the faithful
   `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), wiring the `roundtime` cvar
   (currently a fixed 120s), the full `RestartRound`@0x67920 (game-mode member + per-player round
   setup), `CTSObjective::Freeze`→`ShowObjectiveOnHud`, and the just-respawned grace + the CTB
   (mode-3) and last-man-standing branches in CheckDeads. These are fidelity polish, not new systems.
4. The **CTF/escape entities** (all need teams; ≈0 stock BSP placements → synthetic-verify only):
   ✅ **`CTSFlag` DONE (cont.38, `dlls/ts_flag.cpp`)** — the carry/capture/drop CTF loop, team-gated.
   ✅ **`CTSBriefcase` DONE (cont.39, `dlls/ts_briefcase.cpp`)** — the map-placed CTF base/capture
   zone: seeds its team's CTSFlag at base (FlagFall) + captures a carrier at their own briefcase.
   Verified on the real stock map **`ctb_deadheat`** (2 briefcases each seed their flag, team 0 + 1).
   This replaces CTSFlag's `tsspawnflag` console stand-in for CTB maps.
   ✅ **`CTSBomb` DONE (cont.40, `dlls/ts_bomb.cpp`)** — the defuse/wire-cut objective: a planted
   bomb counts down + explodes unless defused (hold USE, or cut the right of 3 wires via the
   `tscutwire <n>` console command — the wire UI is cl_dll). Synthetic-verified (0 stock placements).
   ✅ **`CTSSellZone` DONE (cont.41, `dlls/ts_sellzone.cpp`)** — sells a dropped weapon (ts_worldgun)
   touching it for cash to the nearest player (the one place the cont.36 cash flows). Verified on the
   real map `ts_dust2`. (Also fixed a pre-existing crash there: the dynamic powerup spawner
   runtime-precached `models/powerup.mdl` → moved to map-load precache.)
   ✅ **`CTSTeamEscape` DONE (cont.42, `dlls/ts_escape.cpp`)** — escort zone, completes when
   `escapepercentage` of the attacker team is simultaneously inside.
   ✅ **`CTSScoreGiver` + `CVipEscape` DONE (cont.43, `dlls/ts_objectives.cpp`)** — `ts_scoregiver`
   = a repeatable team-score giver; `func_vip_safetyzone` = a passive VIP safety-zone brush marker.
   **→ The Track A FGD entity list is COMPLETE.** What's left in Track A is faithful polish, not new
   entities: `UpdateTeamScores`/`InitTeamScore` (vs the current `m_teamScore[]`), the round timer
   (`gmsgTSRoundTime`) + the full `RestartRound`@0x67920, `CTSObjective::Freeze`→`ShowObjectiveOnHud`,
   the CheckDeads grace/CTB/last-man branches, and the fact that CTF capture scores via `TeamWin`
   (which restarts the round) + CTSBomb's detonation just blasts. **Non-FGD leftovers:**
   ✅ `CTSPlayerAmount` + `CTSSuperJump` DONE (cont.44). Remaining: `CInfoBreakable` (a respawning
   breakable — thin, low value) and `CBulletManager` (the traveling-bullet / bullet-time system —
   substantive but DEFERRED: it would rewrite the verified instant-hitscan fire path and isn't visible
   without a cl_dll; best done alongside Track B so the slow bullets can actually be seen).

**Track B — the cl_dll (the client half of TS).** ✅ **FOUNDATION DONE (cont.46): the client now
BUILDS, both 32-bit and 64-bit** — `scripts/build.sh <arch> client` (or `all`). Three gcc-8.3
portability fixes (memory_resource shim, GL enum defines, a 64-bit pointer cast) + dual-arch VGUI:
i386 links Valve's `vgui.so`, amd64 links openvgui's ABI-compatible `libvgui.so` (vendored
`linux/release/vgui64.so`, from xash3d-fwgs) + a 64-bit SDL2. No TS client code yet — the stock HL
client is the base, mirroring how the server started. **VERIFICATION CAVEAT:** this headless box can
build but not RENDER the client (no graphics; the browser client is the separate WASM xash build),
so TS client work is build/load-verified here — visual behaviour needs a graphical client run by the
user. The server already registers + sends the user messages; the client just needs the TS readers.
This unlocks:
- The **visible slow-motion time-dilation** — the server state is complete (`GoSlow`/`InSlow`/
  `CTSSlowMotionPoint`/`GlobalSlowMotion2`), but nothing *reads* the slow fields to scale time. The
  client `pm_shared`/prediction must scale frametime by `m_flSlowMotionGo`; also the motion-blur HUD
  (`gmsgTSMotionBlur`/`gmsgTSPowerUpTime`), the ammo/fire-mode HUDs, the objective HUD pointer.
- The **particle system**: `CTSSharedEmitter` (12) + `CTSEmitter`/`CTSParticle` (the `ts_emitter`/
  `ts_particle` Can't-inits, ~32 placements — the most common remaining) + `CTSWind` (snow), all via
  `gmsgTSParticleSystem`. Needs the cl_dll renderer.
- `CTSStunt` (25 — the signature acrobatics: dives/rolls/wall-flips) is movement prediction →
  client-entangled; `CTSAnimation` (14) similar. `reference/client.dll` (~1,013 exports) is the
  spine — see CLAUDE.md §2.

**Small server-side leftovers** (optional, low value): `CTSPlayerAmount` (player-count relay),
`CTSScoreGiver`, `CTSSuperJump`, `CInfoBreakable`, `CBulletManager`; the per-weapon leaf classes
(`CTSPistolAmmo` etc.) are redundant with the unified `CTSGun`/`ts_weapons` table.

Loop per class: decompile → write `dlls/*.cpp` → add to `dlls/CMakeLists.txt` →
`scripts/build.sh amd64` → `cp` into the rig → boot a map that uses it (frequency
table in `reference/RECONSTRUCTION.md`) → confirm `Can't init <class>` gone (+ temp
`ALERT` probe / bots for behaviour) → **gdb any crash** → revert temp scaffolding,
rebuild clean → commit + update docs.

## Live gotchas (don't re-learn these)
- **gcc2 vs modern ABI**: original has vptr@0x60, gnu-v2 mangling, cdecl-this,
  int-return virtuals. Don't chase those — write natural modern C++. Recovered
  offsets in decompiles are provenance comments only.
- **Decompiler artifacts**: GCC2 pointer-to-member-functions are 8 bytes — you'll
  see paired writes like `this+0x14 = 0xffff0000; this+0x18 = <func>` for one
  `SetTouch`/`SetThink`. Heavy stack-pointer arithmetic (e.g. distance loops) is
  often best reconstructed from *intent*, not transcribed.
- **`dlls/CMakeLists.txt` is an explicit source list** (not glob) — a new file
  won't compile until you add it, but also can't break the build by just existing.
- **`gnv2demangle.py`**: ~74% coverage; misses some array/edge-case mangling and
  has ~60 approximate `T?` backrefs. Cross-check against the decompiled body.
- **Conventions**: NO `Co-Authored-By` trailers. In auto mode, **halt and ask** on
  any blocked/permission situation — don't work around it (e.g. `CLAUDE.md` is in
  the user's global gitignore — leave it local-only, never force-add).
- **Authorization**: original author **Vino ("Vinoisnotozou")** OK'd this; source
  is lost. (`reference/README.md`)

## Keep the docs current (this file is the living pickup brief)
**Context runs out between sessions — a fresh agent must be able to resume from
this file alone.** So at every checkpoint/commit, update *all three* together:
- **`HANDOFF.md`** (this file) — rewrite "Where things stand" + "Recommended next
  steps" to match reality. If it disagrees with the code/git log, it's a bug — fix
  it. Don't let it drift to "predates the last N commits".
- **Serena ledger** (`write_memory("ledger", …)`, newest on top) — append what was
  done, how verified (with the runtime evidence), what's next.
- **`reference/RECONSTRUCTION.md`** — the live status table + frequency/dep tables.

Rule of thumb: if you just committed code, you're not done until these three
reflect it. (`CLAUDE.md` is local-only/gitignored — it carries stable
mission/tooling, but the *durable, tracked* pickup state lives in the three above.)
