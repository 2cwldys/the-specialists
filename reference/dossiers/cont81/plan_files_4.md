# GROUP 4 move plan — TS_worldgun.cpp + hldmcompat.cpp

Read-only planning output. All paths relative to `/home/stevenlafl/Projects/goldsrc.net/mod-ts`.
Line numbers are against the current tree (branch `restructure-source-parity`, HEAD `05be983`).

## Ground truth (verified against funcfile.tsv + ts_inventory.json + nm of reference/ts_i386.so)

**Original `TS_worldgun.cpp`** contains FOUR classes, in this binary-address order
(factory addrs: `TSWorldGun`@0x907c8 < `TSAmmoPack`@0x91310 < `ts_groundweapon`@0x9192c < `ts_groundammo`@0x926f4):

1. `CTSWorldGun`   (factory `TSWorldGun`)
2. `CTSAmmoPack`   (factory `TSAmmoPack`)
3. `CTSGroundWeapon` (factory `ts_groundweapon`)
4. `CTSGroundAmmo` (factory `ts_groundammo`)

**Original `hldmcompat.cpp`** contains 17 classes (16 spawner subclasses + `CTSSuperJump`) and 17
factory exports. Original class order (by `Spawn__` symbol address): SniperAmmo, SniperWeapon,
ShotgunAmmo, ShotgunWeapon, RifleAmmo, RifleWeapon, SMGAmmo, SMGWeapon, PistolAmmo, PistolWeapon,
BigPistolAmmo, BigPistolWeapon, MGWeapon, KnifeWeapon, KatanaWeapon, GrenadeWeapon, **CTSSuperJump (last)**.
Original factory for CTSSuperJump is **`item_longjump`** (0xb9484) — the original does NOT export
`ts_superjump` (see RISKS/follow-ups; not part of this move).

**AUDIT.md FILE mismatches this plan clears (lines 72, 73, 81):**

| class | original | ours today |
|---|---|---|
| `CTSGroundAmmo` | `TS_worldgun.cpp` | dlls/hldmcompat.cpp, dlls/ts_groundammo.cpp |
| `CTSGroundWeapon` | `TS_worldgun.cpp` | dlls/hldmcompat.cpp, dlls/ts_gamerules.cpp, dlls/ts_groundweapon.cpp |
| `CTSSuperJump` | `hldmcompat.cpp` | dlls/ts_powerup.cpp |

(The hldmcompat.cpp / ts_gamerules.cpp attributions for the Ground* classes are audit-regex
artifacts — explicit base calls `CTSGroundWeapon::Spawn()` and even the *comment*
`// CTSGroundWeapon::Fall (RespawnEntities)` at ts_gamerules.cpp:16 match `our_class_files()`'s
`C\w+::\w+\s*\(` regex. They don't need to move; `file_ok = any(...)` passes once the real
definitions live in a file that normalizes to `TS_worldgun.cpp`.)

**No MISSING free functions** are attributed to `TS_worldgun.cpp` or `hldmcompat.cpp` in AUDIT.md
(§ "MISSING free functions") and ts_inventory.json shows `globals: []` for both — so this group is
**pure consolidation**, no blocked-on-RE scaffolding needed.

**Case/naming:** `audit.py:norm_file()` lowercases, drops dir+ext, and `lstrip("ts_")`s — so our
existing lowercase `dlls/ts_worldgun.cpp` already normalizes equal to the original `TS_worldgun.cpp`
("worldgun" == "worldgun"). **No file rename needed**; we merge INTO the existing `dlls/ts_worldgun.cpp`.

---

## 1. EXACT target layout

### dlls/ts_worldgun.cpp  (≡ original TS_worldgun.cpp) — absorbs 2 files

Final section order **matches the original's address order**:

```
banner comment (merge the three file banners)
includes (union, see §2 Move A/B)
1. CTSWorldGun      — already there (lines 27–259, unchanged)
2. CTSAmmoPack      — already there (lines 261–387, unchanged)
3. CTSGroundWeapon  — MOVED IN from dlls/ts_groundweapon.cpp (whole file)
4. CTSGroundAmmo    — MOVED IN from dlls/ts_groundammo.cpp (whole file)
5. extern "C" ShouldCollide — currently lines 389–403; OPTIONAL Move D relocates it
   to dlls/cbase.cpp (the original defines the global ShouldCollide__FP7edict_sT0
   in cbase.cpp per funcfile.tsv:902; cbase.cpp:96–97 already forward-declares it)
```

Files **deleted** after the merge: `dlls/ts_groundweapon.cpp`, `dlls/ts_groundammo.cpp`.

### dlls/hldmcompat.cpp  (≡ original hldmcompat.cpp) — absorbs CTSSuperJump

`CTSSuperJump` class definition + `LINK_ENTITY_TO_CLASS` move here from `dlls/ts_powerup.cpp`.
Placement: **after** the last spawner (`LINK_ENTITY_TO_CLASS(ammo_crossbow, CTSSniperAmmo)` line 229),
**before** the invented `hldmcompat_keep_unlinked()` anchor (lines 231–239) — CTSSuperJump is the
LAST original class in the file; our anchor is an invention and stays at the very end.

Optional cosmetic (not required by audit — it matches by file, not order): reorder hldmcompat's
classes to the original order listed above (ammo-before-weapon pairs, Sniper→…→Grenade→SuperJump).
Defer unless doing a dedicated tidy pass; it is a big diff for zero audit gain.

### Headers — KEEP ours as-is

`ts_inventory.json` `files{}` contains **no .h entries** and funcfile.tsv has no header rows — the
original's header layout is unrecoverable from the binary. `audit.py` file-checks classes by their
**defining .cpp** (and `file_ok` needs only ONE of a class's files to match), so our per-class
headers are invisible to the FILE-mismatch table. Therefore:

- `dlls/ts_worldgun.h` (CTSWorldGun + CTSAmmoPack) — keep.
- `dlls/ts_groundweapon.h` (CTSGroundWeapon; needed by hldmcompat.cpp subclasses + ts_gamerules.cpp) — keep.
- `dlls/ts_groundammo.h` (CTSGroundAmmo; needed by hldmcompat.cpp) — keep.
- `dlls/ts_powerup.h` — keep (CTSSuperJump's base CTSPowerUp lives here).

(If a later pass wants a single `ts_worldgun.h`, it must update includers hldmcompat.cpp:21–22 and
ts_gamerules.cpp:16 — noted, not planned here.)

---

## 2. Every fragment that moves (source file:lines → target)

### Move A — dlls/ts_groundweapon.cpp (ALL 230 lines) → dlls/ts_worldgun.cpp §3

Append after the current CTSAmmoPack section (after line 387), before the ShouldCollide hook.

| # | fragment | src lines | notes |
|---|---|---|---|
| A1 | file banner comment | 1–15 | fold into a `//===` section banner inside ts_worldgun.cpp |
| A2 | includes | 16–22 | dedupe: extdll/util/cbase/player/ts_weapons already in ts_worldgun.cpp; **ADD to ts_worldgun.cpp: `#include "ts_gamerules.h"`** (CashBased()/TSGameRules()/IsTeamMode/GetTeamIndex in GetWeapon) **and `#include "ts_groundweapon.h"`** |
| A3 | `#define SF_GW_ON_WALL 16` | 24 | file-scope macro; no collision (see §5) |
| A4 | `LINK_ENTITY_TO_CLASS(ts_groundweapon, CTSGroundWeapon)` | 26 | factory export `ts_groundweapon` |
| A5 | `TYPEDESCRIPTION CTSGroundWeapon::m_SaveData[]` | 28–32 | save table |
| A6 | `IMPLEMENT_SAVERESTORE(CTSGroundWeapon, CBaseEntity)` | 33 | emits Save/Restore |
| A7 | `CTSGroundWeapon::KeyValue` | 35–43 | |
| A8 | `CTSGroundWeapon::Precache` (+ comment) | 45–51 | |
| A9 | `CTSGroundWeapon::Spawn` | 53–84 | |
| A10 | `CTSGroundWeapon::PutOnWall` | 87–116 | uses `M_PI`/`cos`/`sin` (mathlib via extdll/util — already fine in ts_worldgun.cpp) |
| A11 | `CTSGroundWeapon::Fall` | 119–140 | EXPORT think |
| A12 | `CTSGroundWeapon::OnGround` | 142–150 | |
| A13 | `CTSGroundWeapon::WaitThink` | 152–155 | |
| A14 | `CTSGroundWeapon::Use` | 158–161 | |
| A15 | `CTSGroundWeapon::WaitTouch` | 164–168 | |
| A16 | `CTSGroundWeapon::GetWeapon` | 171–230 | the buy-gate block references ts_gamerules.h symbols |

Then `git rm dlls/ts_groundweapon.cpp`.

### Move B — dlls/ts_groundammo.cpp (ALL 156 lines) → dlls/ts_worldgun.cpp §4

Append after the CTSGroundWeapon section (original order: CTSGroundAmmo is last).

| # | fragment | src lines | notes |
|---|---|---|---|
| B1 | file banner comment | 1–21 | fold into section banner |
| B2 | includes | 22–28 | dedupe: everything already present after Move A **except `#include "ts_groundammo.h"` — ADD** (weapons.h and ts_weapons.h are already in ts_worldgun.cpp:22–23) |
| B3 | `#define AMMOPACK_MODEL "models/ammopack.mdl"` | 30 | no clash — ts_worldgun.cpp uses the string literal directly at line 298 (CTSAmmoPack::Spawn); optionally have that line reuse the macro after the move |
| B4 | `#define SF_AMMO_ON_WALL 16` | 31 | distinct name from SF_GW_ON_WALL; both value 16, no clash |
| B5 | `LINK_ENTITY_TO_CLASS(ts_groundammo, CTSGroundAmmo)` | 33 | factory export `ts_groundammo` |
| B6 | `CTSGroundAmmo::KeyValue` | 35–53 | |
| B7 | `CTSGroundAmmo::Precache` | 55–60 | |
| B8 | `CTSGroundAmmo::Spawn` | 62–78 | |
| B9 | `CTSGroundAmmo::Fall` | 80–100 | EXPORT think |
| B10 | `CTSGroundAmmo::OnGround` | 102–118 | |
| B11 | `CTSGroundAmmo::PutOnWall` | 120–124 | |
| B12 | `CTSGroundAmmo::WaitThink` | 126–130 | |
| B13 | `CTSGroundAmmo::WaitTouch` | 132–156 | |

Then `git rm dlls/ts_groundammo.cpp`.

### Move C — CTSSuperJump: dlls/ts_powerup.cpp:441–456 → dlls/hldmcompat.cpp

| # | fragment | src lines | target |
|---|---|---|---|
| C1 | banner comment `// CTSSuperJump — …` | ts_powerup.cpp 441–445 | hldmcompat.cpp, after line 229 |
| C2 | `class CTSSuperJump : public CTSPowerUp { … }` (inline Spawn) | 446–454 | same |
| C3 | `LINK_ENTITY_TO_CLASS(ts_superjump, CTSSuperJump)` | 456 | same (keep the `ts_superjump` name for now — behavior-preserving; see RISKS for the item_longjump contract follow-up) |
| C4 | includes to ADD to hldmcompat.cpp (after line 22) | — | `#include "ts_gamerules.h"` **then** `#include "ts_powerup.h"` — ORDER MATTERS: ts_powerup.h:28–29 consumes `TS_PWUP_*` macros defined in ts_gamerules.h (TS_PWUP_LOWGRAV = ts_gamerules.h:30) and documents "include this header after it" |

ts_powerup.cpp needs no other edit (nothing else in it references CTSSuperJump; verified by grep).

### Move D (OPTIONAL, source-parity bonus) — global ShouldCollide → dlls/cbase.cpp

| # | fragment | src lines | target |
|---|---|---|---|
| D1 | `extern "C" int ShouldCollide(edict_t*, edict_t*)` definition + banner | ts_worldgun.cpp 389–403 | dlls/cbase.cpp, replacing the forward decl at cbase.cpp:96–97 (keep it above `gNewDLLFunctions`, line 99) |

Original: `ShouldCollide__FP7edict_sT0` lives in **cbase.cpp** (funcfile.tsv:902). Needs
`ts_worldgun.h` visible in cbase.cpp (for the `CTSWorldGun` cast) — add
`#include "ts_worldgun.h"` there (cbase.cpp already includes cbase/util/player chain).
audit.py's free-func diff is per-TS-file, so this is cosmetic for the audit but real for parity.
Can be skipped without affecting the FILE-mismatch table.

**Fragment count: A(16) + B(13) + C(4) + CMake(2) + D(1) ≈ 36 (≈31 without the optional D and
counting the include edits once).**

---

## 3. dlls/CMakeLists.txt edits

Current (lines 129–131, 139, 142):

```
	ts_groundammo.cpp      <- DELETE line 129
	ts_groundweapon.cpp    <- DELETE line 130
	hldmcompat.cpp         <- keep
	...
	ts_powerup.cpp         <- keep
	...
	ts_worldgun.cpp        <- keep
```

Only the two deletions. No cl_dll/CMakeLists.txt changes (no client files in this group;
`cl_dll/ts_powerup_hud.cpp` is untouched).

---

## 4. Ordering + verification (build stays green after every step)

Baseline first: `scripts/build.sh amd64` green; capture
`nm -D build-amd64/dlls/hl.so | awk '$2=="T"' > /tmp/nm_before.txt`.

**Step 1 — Move A** (groundweapon → ts_worldgun.cpp, CMake delete, `git rm`):
- `scripts/build.sh amd64`
- `nm -D build-amd64/dlls/hl.so | grep -E ' T (TSWorldGun|TSAmmoPack|ts_groundweapon)$'` → 3 hits
- `nm build-amd64/dlls/hl.so | c++filt | grep 'CTSGroundWeapon::'` → Spawn/Precache/KeyValue/Save/Restore/Fall/PutOnWall/OnGround/WaitThink/Use/WaitTouch/GetWeapon all present

**Step 2 — Move B** (groundammo → ts_worldgun.cpp, CMake delete, `git rm`):
- build; `nm -D … | grep ' T ts_groundammo$'`; `nm … | c++filt | grep 'CTSGroundAmmo::'`

**Step 3 — Move C** (CTSSuperJump → hldmcompat.cpp):
- build; `nm -D build-amd64/dlls/hl.so | grep ' T ts_superjump$'`
- `nm build-amd64/dlls/hl.so | c++filt | grep 'CTSSuperJump::'` (Spawn is a W/weak inline symbol — that's fine, it was inline before the move too)

**Step 4 — (optional) Move D**; build; `nm -D … | c++filt | grep -w ShouldCollide` still exported.

**Final gate:**
- `nm -D build-amd64/dlls/hl.so | awk '$2=="T"' > /tmp/nm_after.txt; diff /tmp/nm_before.txt /tmp/nm_after.txt` → **empty** (no export gained/lost; addresses excluded by taking only the name column: use `awk '$2=="T"{print $3}' | sort`)
- All 21 group-4 factories present:
  `TSWorldGun TSAmmoPack ts_groundweapon ts_groundammo ts_superjump weapon_9mmAR weapon_shotgun weapon_gauss weapon_egon weapon_rpg ammo_ARgrenades weapon_satchel weapon_crossbow weapon_hornetgun weapon_snark weapon_handgrenade ammo_9mmAR ammo_gaussclip ammo_rpgclip ammo_buckshot ammo_crossbow`
- `python3 scripts/coverage/audit.py` → FILE-mismatch rows for CTSGroundAmmo / CTSGroundWeapon / CTSSuperJump gone (AUDIT.md regenerates)
- Behaviour smoke (unchanged code, but cheap): spawn `ts_groundweapon` + touch a `weapon_shotgun`
  spawner on a DM map in the rig — pickups still work.

Each step is an independent green-build commit candidate.

---

## 5. RISKS

1. **Include order (real, documented):** `ts_powerup.h` REQUIRES `ts_gamerules.h` before it
   (TS_PWUP_* macros; ts_powerup.h:28 says so, and ts_powerup.cpp:32/39 does exactly that). In
   hldmcompat.cpp add `ts_gamerules.h` first, then `ts_powerup.h`.
2. **ts_worldgun.cpp gains a ts_gamerules.h dependency** (CTSGroundWeapon::GetWeapon buy gate:
   CashBased/TSGameRules/IsTeamMode/GetTeamIndex, plus `pPlayer->m_iCash`/`GiveCash` from player.h —
   already included). Forgetting it = compile error, caught at Step 1.
3. **File-scope statics/macros — checked, NO collisions:** none of ts_worldgun.cpp /
   ts_groundweapon.cpp / ts_groundammo.cpp defines file-scope statics; hldmcompat.cpp's
   `static RerollModel` stays put; macros `SF_GW_ON_WALL`(16) / `SF_AMMO_ON_WALL`(16) /
   `AMMOPACK_MODEL` have distinct names and don't pre-exist in the target. Same-NAME methods
   (Fall/OnGround/WaitThink/WaitTouch/PutOnWall × 4 classes in one TU) are class-qualified — fine
   (CTSWorldGun+CTSAmmoPack already coexist this way).
4. **Factory-name contract follow-ups surfaced by this group (do NOT fold into the move):**
   (a) the original links CTSSuperJump to **`item_longjump`** and never exports `ts_superjump`
   (coverage REPORT.md lists `ts_superjump` as EXTRA). Fixing means removing/repointing the stock
   `LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump)` at dlls/items.cpp:339 — otherwise a
   duplicate C symbol `item_longjump` link error. Behavioral change; separate change + runtime test.
   (b) original exports **`weapon_9MMAR`** (uppercase MM, funcfile.tsv/inventory) vs our
   `weapon_9mmAR` (hldmcompat.cpp:65); engine classname lookup is exact-case. Separate contract fix.
5. **References to the deleted filenames:** dlls/CMakeLists.txt (edited here);
   `reference/coverage/{AUDIT,LAYOUT_MAP,INVENTORY,SDK_DIVERGENCE}.md` are GENERATED — regenerate
   via audit.py, don't hand-edit; `reference/RECONSTRUCTION.md` + `.serena/memories/ledger.md`
   mention the old files historically (optional doc touch-up); no scripts reference them
   (grep of scripts/ is clean). `ts_gamerules.cpp:16` keeps its `#include "ts_groundweapon.h"`.
6. **Audit-regex artifact awareness:** hldmcompat.cpp will STILL be listed by
   `our_class_files()` for CTSGroundWeapon/CTSGroundAmmo (base-call syntax matches the regex), and
   ts_gamerules.cpp for CTSGroundWeapon (a comment matches). Harmless — `file_ok=any()` passes via
   ts_worldgun.cpp — but don't be surprised if a per-class `audit.py CTSGroundWeapon` view shows
   multiple files.
7. **Git history:** these are content moves + deletions, not renames — `git log --follow` on
   ts_worldgun.cpp won't show the groundweapon/groundammo history. Acceptable; note in the commit
   message which files merged in.
8. **cl_dll untouched** — nothing in this group builds into client.so; only hl.so nm checks apply.
