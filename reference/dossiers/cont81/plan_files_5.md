# GROUP 5 move plan — TS_bullet.cpp + TS_emitter.cpp + TS_grenades.cpp

Branch: `restructure-source-parity`. READ-ONLY plan (nothing moved yet).

Goal: clear 3 FILE-mismatch rows in `reference/coverage/AUDIT.md` (`CBulletManager`,
`CTSFog`, `CTSKnife`). The audit matches by file name via `audit.py:norm_file` =
basename, lowercased, ext dropped, `lstrip("ts_")`, `"manager"` removed — **verified by
running the real normalizer**:

| original | our target name | norm(orig) == norm(target) |
|---|---|---|
| `TS_bullet.cpp`   | `dlls/ts_bullet.cpp`   | `bullet` == `bullet` ✓ (current `ts_bulletmgr.cpp` → `bulletmgr` ✗) |
| `TS_emitter.cpp`  | `dlls/ts_emitter.cpp`  | `emitter` == `emitter` ✓ (already exists; `ts_fog.cpp` → `fog` ✗) |
| `TS_grenades.cpp` | `dlls/ts_grenades.cpp` | `grenades` == `grenades` ✓ (already exists; `ts_knife.cpp` → `knife` ✗) |

Case: the normalizer is case-insensitive, and the repo convention is lowercase `ts_*.cpp`
(ts_gun.cpp, ts_gamerules.cpp, …) — so plan lowercase `ts_bullet.cpp`. If the repo-wide
restructure decision becomes "adopt original exact case (`TS_*.cpp`)", the same moves
apply with two extra case-only `git mv`s for ts_emitter/ts_grenades; audit outcome is
identical either way.

The audit scans **out-of-line `Class::Method` definitions in .cpp files only**
(`audit.py:our_class_files`), so headers do NOT need renaming for the audit — header
handling below is consistency-only.

Original file contents (ground truth, `funcfile.tsv` + `ts_inventory.json` `files`):

- **TS_bullet.cpp**: class `CBulletManager` (ctor/dtor, Reset, Think, FireBullet,
  DestroyBullet, UpdateBullet, UpdateBulletTimeSlice) + free `UTIL_TraceLine_Expanded`
  (@0x5f4f4, AUDIT-missing — needs RE) + global `glast_weaponid` (nm: D @0x10100c,
  4 bytes; its static-init `_GLOBAL_.I.glast_weaponid` is in this TU — we don't define it).
- **TS_emitter.cpp**: classes `CTSEmitter`, `CTSParticle`, `CTSWind`, `CTSFog` + the four
  factories `ts_emitter`/`ts_particle`/`ts_wind`/`ts_fog`. (CTSSharedEmitter =
  TS_sharedemitter.cpp — our `ts_sharedemitter.cpp` already matches; untouched.)
- **TS_grenades.cpp**: classes `CTSPhysicObject`, `CTSGrenade`, `CTSShrapnel`,
  `CM61Grenade`, `CTSKnife` + free `TSExplosion` + factories `shrapnel`, `m61_grenade`
  (AUDIT-missing in ours — see follow-ups), `knife`. Original in-file/address order:
  CTSPhysicObject → CTSGrenade → CTSShrapnel → CM61Grenade → TSExplosion → CTSKnife (last).

---

## 1. Target layout

| our current file | action | target |
|---|---|---|
| `dlls/ts_bulletmgr.cpp` (211 L, whole file) | **git mv (rename)** | `dlls/ts_bullet.cpp` |
| `dlls/ts_bulletmgr.h` (38 L) | **git mv (rename, consistency only)** | `dlls/ts_bullet.h` — optional for the audit (headers unscanned) but keeps the pair named alike; 3 include sites to update (§2A) |
| `dlls/ts_fog.cpp` (26 L) | **merge → delete** | append `CTSFog` block at the END of `dlls/ts_emitter.cpp` (fog is last in the original too) |
| `dlls/ts_knife.cpp` (334 L) | **merge → delete** | append `CTSKnife` section at the END of `dlls/ts_grenades.cpp` (matches original order, CTSKnife last) |
| `dlls/ts_emitter.cpp`, `dlls/ts_grenades.cpp` | keep (names already match) | receive merged content |
| `dlls/ts_physicobject.h`, `dlls/ts_sharedemitter.{h,cpp}` | keep unchanged | — |

**Headers policy:** `ts_inventory.json` carries NO header-level data (the binary yields
per-TU `.cpp` attribution only — original `.h` names are unknowable). Keep all our
headers; only `ts_bulletmgr.h → ts_bullet.h` is renamed, to track its .cpp. Note this in
the file banners.

## 2. Exact fragment moves

### A. `dlls/ts_bulletmgr.cpp` → `dlls/ts_bullet.cpp` (pure rename; content unchanged except banner + one include)

Whole file moves; enumerated so nothing is silently dropped:

| fragment | src lines (ts_bulletmgr.cpp) | target |
|---|---|---|
| banner comment (retitle to `ts_bullet.cpp`, add "= original TS_bullet.cpp") | 1–18 | ts_bullet.cpp top |
| includes (change `#include "ts_bulletmgr.h"` → `"ts_bullet.h"`) | 19–28 | ts_bullet.cpp |
| `CBulletManager g_BulletManager;` global definition | 30 | ts_bullet.cpp |
| provenance comments | 32–36 | ts_bullet.cpp |
| `static const int kBulletSpeedUnit[16]` (file-scope static) | 42–44 | ts_bullet.cpp |
| `static float BulletFullSpeed(int)` (file-scope static) | 45–51 | ts_bullet.cpp |
| `CBulletManager::DestroyBullet` | 53–65 | ts_bullet.cpp |
| `CBulletManager::FireBullet` | 67–112 | ts_bullet.cpp |
| `CBulletManager::Reset` | 114–122 | ts_bullet.cpp |
| `CBulletManager::Think` | 124–130 | ts_bullet.cpp |
| `CBulletManager::UpdateBullet` | 132–155 | ts_bullet.cpp |
| `CBulletManager::UpdateBulletTimeSlice` | 157–211 | ts_bullet.cpp |

Header rename `ts_bulletmgr.h → ts_bullet.h` (contains `struct TSSlowBullet`,
`class CBulletManager`, `extern CBulletManager g_BulletManager`; rename include-guard
`TS_BULLETMGR_H` → `TS_BULLET_H`). Include-site updates — exactly 3 code sites:
- `ts_bullet.cpp:27` (was ts_bulletmgr.cpp:27)
- `dlls/ts_gun.cpp:23`
- `dlls/ts_gamerules.cpp:12`

Comment-only mentions of the old name (not build-relevant; update opportunistically):
`dlls/ts_player.h:22`, `dlls/ts_weapons.h:50`, `dlls/ts_weapons.cpp:42,47,48`,
`dlls/ts_gun.cpp:1358`, `dlls/ts_gamerules.cpp:700`, `dlls/ts_shared.cpp:7`.
(HANDOFF.md lines 1090/1128/1316 mention ts_bulletmgr.cpp/ts_knife.cpp — historical
ledger text; leave, or add a one-line "renamed in restructure" note.)

Future in-file additions for ts_bullet.cpp (blocked on RE — record in the banner):
- free `UTIL_TraceLine_Expanded(const Vector&, const Vector&, IGNORE_MONSTERS, edict_t*, TraceResult*)` @0x5f4f4.
- global `int glast_weaponid` (defined in this TU per the binary; its siblings
  glast_closecombat/coldcock/headshot live elsewhere — TS_player.cpp region per nm).

### B. `dlls/ts_fog.cpp` → merge into `dlls/ts_emitter.cpp`, then delete

`ts_emitter.cpp` currently ends at line 176 (`CTSWind::BlowThink`). Append a CTSFog
section. All of ts_fog.cpp's includes (extdll/util/cbase, lines 11–13) already exist in
ts_emitter.cpp lines 10–12 → dropped.

| fragment | src lines (ts_fog.cpp) | target (ts_emitter.cpp) |
|---|---|---|
| banner (fold the "@PointClass ts_fog / self-removes on Spawn / keyvalues near-far-color are client-side" note into a `//====` section header; also add CTSFog to the ts_emitter.cpp banner class list, line 2) | 1–10 | new section comment, end of file |
| `class CTSFog : public CBaseEntity { void Spawn() override; };` | 15–19 | end of file |
| `LINK_ENTITY_TO_CLASS(ts_fog, CTSFog);` | 21 | end of file |
| `CTSFog::Spawn()` (`UTIL_Remove(this)`) | 23–26 | end of file |

Then `git rm dlls/ts_fog.cpp`. ts_fog.cpp has **no file-scope statics → no collision
possible**. ts_emitter.cpp's existing static `RegisterParticleMsg()` (16–20) untouched.
NB `scripts/coverage/coverage.py:401 SELF_REMOVING_ENTITIES = {"ts_fog"}` names the
ENTITY classname, not the file — unaffected.

### C. `dlls/ts_knife.cpp` → merge into `dlls/ts_grenades.cpp`, then delete

`ts_grenades.cpp` currently ends at line 406 (`TSThrowM61`). Append the whole CTSKnife
section AFTER it (original order: CTSKnife last in TS_grenades.cpp).

Includes: ts_knife.cpp has `extdll.h, util.h, cbase.h, player.h, ts_weapons.h, ts_gun.h`
(lines 17–22). ts_grenades.cpp already has all but **`ts_gun.h`** → add
`#include "ts_gun.h"` to the ts_grenades.cpp include block (after `ts_weapons.h`,
~line 19; supplies the TSW_KNIFE/TSW_SEALKNIFE/TSW_KATANA ids CTSKnife uses).

| fragment | src lines (ts_knife.cpp) | target (ts_grenades.cpp, appended in this order) |
|---|---|---|
| banner (fold into a `//==== CTSKnife …` section comment; keep the "ORIGINAL SOURCE: TS_grenades.cpp" + RE provenance; ALSO fix ts_grenades.cpp's own banner — line 2 still says "ts_grenade.cpp" and mentions only CM61Grenade) | 1–16 | section comment |
| `class CTSKnife : public CBaseEntity {…}` (incl. inline Classify/ObjectCaps/ShouldCollide/StabDamage/Gravity/HitSound/BodySound + members m_knifeType/m_flDamage/m_flExpire) | 24–57 | after TSThrowM61 |
| `LINK_ENTITY_TO_CLASS(knife, CTSKnife);` | 59 | " |
| `CTSKnife::Spawn` | 61–83 | " |
| `CTSKnife::WaitThink` | 85–119 | " |
| `CTSKnife::BecomePickup` | 121–155 | " |
| `static int KnifeHitPitch(CBaseEntity*)` (file-scope static) | 157–165 | " — must precede StickAt/BounceTouch/Use (callers) |
| `CTSKnife::StickAt` | 167–177 | " |
| `static bool KnifeGlances(const Vector&, const Vector&)` (file-scope static) | 179–186 | " — must precede BounceTouch |
| `CTSKnife::BounceTouch` | 188–247 | " |
| `CTSKnife::StartSettle` | 249–261 | " |
| `CTSKnife::SettleThink` | 263–277 | " |
| `CTSKnife::PickupTouch` | 279–289 | " |
| `CTSKnife::CreateKnife` (static factory) | 291–302 | " |
| `CTSKnife::GetDescription` | 304–314 | " |
| `CTSKnife::Use` | 316–328 | " |
| free `CBaseEntity *TSThrowKnife(…)` (declared ts_weapons.h:21) | 330–334 | " — keep last |

Then `git rm dlls/ts_knife.cpp`.

**Static/name collision audit of the merged ts_grenades.cpp:** existing file-scope names
= macros `SHRAPNEL_MODEL`, `SHRAPNEL_DMG`, `M61_SHAKE_MAG`, `M61_BLAST_DAMAGE`,
`M61_BLAST_RADIUS`; `extern int gmsgTSCreateDebris`; classes
CTSGrenade/CTSShrapnel/CM61Grenade (+ CTSPhysicObject methods); free `TSExplosion`,
`TSThrowM61`; one function-local `static const char *const snd[4]`. Incoming from
ts_knife.cpp: statics `KnifeHitPitch`, `KnifeGlances`; class `CTSKnife`; free
`TSThrowKnife`. **No collisions.** Same-named METHODS across classes (WaitThink /
BounceTouch / Spawn / Use / Precache on CTSKnife vs CTSShrapnel/CM61Grenade/CTSGrenade/
CTSPhysicObject) are class-scoped — fine.

Future in-file additions for ts_grenades.cpp (record in banner; NOT part of this move):
- `m61_grenade` @0x6c850 (AUDIT "missing free function") = the original's C entity
  factory, i.e. `LINK_ENTITY_TO_CLASS(m61_grenade, CM61Grenade)`. Trivial to add but it
  touches the spawn/classname contract (ours sets classname "grenade" in CreateGrenade)
  — verify the original's classname via RE first.
- `CTSKnife::WaitTouch` @0x6dce8 (AUDIT missing method — blocked on RE; our think-based
  SettleThink replacement is a documented divergence).
- Context only (pre-existing AUDIT rows, unchanged by the move): CTSKnife's 6 INVENTED
  methods (BecomePickup/Classify/PickupTouch/SettleThink/StartSettle/StickAt).

## 3. CMakeLists.txt edits

`dlls/CMakeLists.txt` only (all three are server files; no cl_dll changes; headers are
not listed in CMake):
- line 111: **delete** `\tts_fog.cpp`
- line 141: `\tts_bulletmgr.cpp` → `\tts_bullet.cpp`
- line 143: **delete** `\tts_knife.cpp`
(`ts_emitter.cpp` line 133 and `ts_grenades.cpp` line 128 already present — unchanged.)

## 4. Ordering + verification (build green after every step)

Baseline (verified on the current build-amd64/dlls/hl.so): factories exported =
`T ts_fog ts_emitter ts_particle ts_wind knife shrapnel`; 23 out-of-line
`CBulletManager::|CTSKnife::|CTSFog::` method symbols.

**Step 1 — bullet rename (1 commit):**
`git mv dlls/ts_bulletmgr.cpp dlls/ts_bullet.cpp && git mv dlls/ts_bulletmgr.h dlls/ts_bullet.h`;
fix 3 `#include` sites + include guard; CMake line 141.
Build `scripts/build.sh amd64`. Checks:
`nm build-amd64/dlls/hl.so | c++filt | grep -c 'CBulletManager::'` ≥ 6 (no entity
factory — CBulletManager isn't an entity); `grep -rn ts_bulletmgr dlls/ cl_dll/` → nothing (or comments only).

**Step 2 — fog merge (1 commit):**
append CTSFog block to ts_emitter.cpp; `git rm dlls/ts_fog.cpp`; CMake line 111.
Build. Checks: `nm -D build-amd64/dlls/hl.so | grep -wE 'T (ts_fog|ts_emitter|ts_particle|ts_wind)'`
→ all 4 factories; `nm … | c++filt | grep 'CTSFog::Spawn'` present.

**Step 3 — knife merge (1 commit):**
append CTSKnife section + `#include "ts_gun.h"` to ts_grenades.cpp; `git rm dlls/ts_knife.cpp`;
CMake line 143.
Build. Checks: `nm -D … | grep -wE 'T (knife|shrapnel)'` both exported;
`nm … | c++filt | grep -c 'CTSKnife::'` == baseline (12 out-of-line);
`nm … | c++filt | grep TSThrowKnife` present (T).

**Final gate:** `python3 scripts/coverage/audit.py` → the FILE-mismatch rows for
`CBulletManager`, `CTSFog`, `CTSKnife` disappear (22 → 19); no NEW missing/invented
methods (the fragment tables are exhaustive). Optionally `scripts/coverage/coverage.py
all` to confirm the external contract (entities/keyvalues) is untouched — the moves are
purely textual, binaries behavior-identical.

## 5. RISKS

- **Include-order**: CTSKnife needs `ts_gun.h` in ts_grenades.cpp (TSW_* ids) — added in
  step 3. Everything else it uses (TSPickUpWeapon/TSSlowPitch via ts_weapons.h, player.h)
  is already included there. The two knife statics must be appended before their callers
  (original source order preserved → safe).
- **Statics collisions**: none (both merge pairs audited above); SHRAPNEL_*/M61_* macros
  don't leak into knife code; ts_fog.cpp brings zero file-scope names.
- **audit normalizer quirk**: `lstrip("ts_")` strips the CHAR SET {t,s,_}, not the literal
  prefix — all three targets verified with the real function; don't generalize this rename
  pattern to files whose stem starts with t/s after the prefix without re-checking.
- **By-name references**: zero scripts reference these .cpp names (grepped scripts/).
  dlls/ comment mentions (7 sites, §2A) + HANDOFF.md history are comment-only.
  `coverage.py`'s `ts_fog` is the entity classname — unaffected.
- **git history**: step 1 is a clean rename (`--follow` works); steps 2–3 are
  append+delete, so blame for the moved code lands on the merge commit — keep each merge
  a separate commit stating "moved verbatim from ts_knife.cpp / ts_fog.cpp".
- **Not regressions** (pre-existing AUDIT rows that intentionally survive this move):
  missing UTIL_TraceLine_Expanded, m61_grenade factory, CTSKnife::WaitTouch, global
  glast_weaponid, CTSKnife's 6 invented methods — all RE work, recorded as future
  in-file additions in the new banners.
- **Runtime re-test convenience**: after rebuild, re-run `scripts/setup-xash-mod.sh`
  before any xash smoke test (arch-suffixed lib names).
