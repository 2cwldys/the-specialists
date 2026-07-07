# cont91 — FindEntityByString crash (metamod + rcbot, TS-only): dangling / premature-removal audit

**Scope:** read-only RE + code review of the `restructure-source-parity` branch.
**Symptom:** engine faults inside `pfnFindEntityByString` (the linear edict scan reading each
edict's `classname` string offset / private data) with metamod + rcbot bots on THE SPECIALISTS,
**never** on stock valve. 45+ min of active bot combat AND rapid map changes did **not** reproduce
→ a specific TS entity-lifecycle edge, not the common find/iterate path.

---

## TL;DR / ranked verdict

| # | Site | What dangles | Confidence it's a real bug | Confidence it's THE crash |
|---|------|--------------|----------------------------|---------------------------|
| **1** | `dlls/ts_worldgun.cpp:195-197` — `CTSWorldGun::WaitThink` **missing-brace** bug | Dropped-weapon (`"WorldGun"`) edicts are `UTIL_Remove`d ~0.1 s after they settle → massive WorldGun edict churn; the non-expired removal path also **skips** `RemoveEntityFromSlowedArray` (latent dangling raw pointer) | **HIGH — CONFIRMED divergence, proven by disasm** | **MEDIUM** (best fit; connection to the engine fault is via edict-slot churn feeding rcbot's cached pointers) |
| 2 | `dlls/ts_worldgun.cpp:196` + `dlls/TS_util.cpp:131` — scrub-skip → `m_pSlowedEnts[]` raw-pointer dangle | Same brace bug removes a WorldGun **without** scrubbing it from every player's `m_pSlowedEnts[32]` raw-pointer array | HIGH the code is wrong | **LOW today** — the slowed array is a deferred SLICE (never populated yet), so the dangle is *latent*; becomes a live UAF the moment the slow-motion entity array lands |
| 3 | `dlls/TS_gamerulesclass.cpp:359-360` — `RespawnEntities` calls `ts_powerup->Spawn()` (VERIFY: orig calls `Fall()`) | Re-running full `Spawn()` on a live, already-linked powerup mid-`FindEntityByClassname` loop | MEDIUM (flagged VERIFY in-source) | LOW — no removal; re-inits but does not free the cursor |
| 4 | Root-cause framing: TS pickup-entity churn (WorldGun/flag/powerup/ammo) vs rcbot's cached raw edict pointers | rcbot caches weapon/pickup edicts and enumerates entities every frame; a freed+reused TS pickup slot makes a cached edict stale | — | This is the *mechanism* by which #1 reaches `pfnFindEntityByString` |

**Bottom line:** the only **confirmed** source-vs-binary divergence in the removal discipline is the
`CTSWorldGun::WaitThink` brace bug (candidate #1/#2). Fix it first. I did **not** find a smoking-gun
in-frame "remove the cursor then engine-`->next`" UAF, because **every** TS-side `Find*` loop uses the
**deferred** `UTIL_Remove` (see "Why the obvious theory is mostly exonerated" below), which keeps the
cursor edict valid for the rest of the frame.

---

## Background: what actually faults `pfnFindEntityByString`, and why deferred removal is safe

`UTIL_Remove` (`dlls/util.cpp:1582`) is **deferred**:

```c
void UTIL_Remove(CBaseEntity* pEntity){
    if(!pEntity) return;
    pEntity->UpdateOnRemove();
    pEntity->pev->flags |= FL_KILLME;   // engine frees the edict later (physics run)
    pEntity->pev->targetname = 0;
}
```

It does **not** free the edict or delete the private data. The engine frees `FL_KILLME` edicts later
(during the physics pass), and only then does `OnFreeEntPrivateData` (`dlls/cbase.cpp:297`) `delete`
the `CBaseEntity` and null `pvPrivateData`. Edict-free + private-data-delete are therefore **atomic**
from the DLL's view — a `UTIL_Remove`d entity never leaves "private data freed but edict still live
with a stale classname."

Consequences for the audit:

- **In-frame cursor removal is safe.** A `while((e=UTIL_FindEntityByClassname(e,...)))` loop whose body
  `UTIL_Remove`s `e` still has a valid `e` edict for the next `FIND_ENTITY_BY_STRING(e,...)` — the
  engine sees an `FL_KILLME`-flagged but *live* edict. No in-frame UAF. This exonerates the classic
  "remove-then-`->next`" pattern for **all** our loops.
- The engine fault therefore needs one of: **(a)** a TS pickup edict that was freed *and its slot
  reused* while a **cached raw pointer/edict** (rcbot, or a TS raw-pointer array) still refers to the
  old occupant, or **(b)** a live edict (`free==0`) whose `pev->classname` is a garbage `string_t`
  (out-of-pool offset). `classname==0` is safe (`STRING(0)==""`).

The core plumbing that would cause (b) directly is **correct**: `OnFreeEntPrivateData`/`EHANDLE`/
`GET_PRIVATE` (`dlls/cbase.cpp:140-334, 489-534`) match stock HL. No TS entity assigns `classname`
from a raw buffer or leaves it garbage; runtime-created TS entities all set `classname` in their
factory or in `Spawn` **before** they can be iterated (`CreateWeapon` sets `"WorldGun"` pre-`Spawn`
@ ts_worldgun.cpp:48; `CreateAmmoPack` `"AmmoPack"` @ :278; `CTSFlag::Spawn` `"ts_flag"` @
ts_flag.cpp:98; `CTSPowerUp::Spawn` `"ts_powerup"` @ ts_powerup.cpp:110). So the realistic path is (a):
**edict-slot churn + a stale cached pointer**, which is exactly what candidate #1 amplifies.

---

## Candidate #1 — `CTSWorldGun::WaitThink` missing-brace: unconditional `UTIL_Remove` (CONFIRMED)

**File:** `dlls/ts_worldgun.cpp:191-198`

```cpp
// @0xa104c WaitThink — idle keep-alive after settling: remove once the lifetime expires.
void CTSWorldGun::WaitThink()
{
    pev->nextthink = gpGlobals->time + 0.1f;
    if (m_flExpire < gpGlobals->time)
        RemoveEntityFromSlowedArray(this); // ← guarded by the if
        UTIL_Remove(this);                 // ← BUG: NOT guarded — runs every tick
}
```

The `if` has no braces, so **only** `RemoveEntityFromSlowedArray(this)` is conditional;
`UTIL_Remove(this)` runs on **every** `WaitThink` tick.

### Proof this is a source-vs-binary divergence (not the original's behaviour)

`nm reference/ts_i386.so` → `WaitThink__11CTSWorldGun @ 0x9104c`. Disassembly:

```
9104c <WaitThink__11CTSWorldGun>:
  ...  fldl -0x6ecd8(%ebx)      ; 0.1
       fadds (%eax)             ; + gpGlobals->time
       fstps 0x104(%ecx)        ; pev->nextthink = time + 0.1
       flds  0xc4(%esi)         ; m_flExpire
       fcomps (%edx)            ; compare gpGlobals->time
       fnstsw %ax ; sahf
       jae   9109c              ; if (m_flExpire >= time) -> SKIP BOTH calls
       push  %esi
       call  RemoveEntityFromSlowedArray@plt
       mov   %esi,(%esp)
       call  UTIL_Remove@plt     ; BOTH calls are INSIDE the m_flExpire<time branch
9109c: add $0x4,%esp ; ret
```

The original gates **both** `RemoveEntityFromSlowedArray` **and** `UTIL_Remove` under
`if (m_flExpire < gpGlobals->time)`. Our reconstruction dropped the braces and moved
`UTIL_Remove` out of the branch.

Sibling removal paths are all correctly braced (checked): `CTSShrapnel::WaitThink`
(ts_grenades.cpp:192-195), `CM61Grenade::WaitThink` (:385-390), `CTSKnife::WaitThink` (:523-528),
`CTSKnife::Use` (:735-739), `CTSFlag::Destroy` (ts_flag.cpp:145-147), and the same file's
`CTSWorldGun::Fall` (:186-188) and `CTSWorldGun::GetWeapon` (:236-237). Only `WaitThink` is wrong —
an isolated regression.

### Why it can dangle / reach the crash

- `OnGround` (ts_worldgun.cpp:200-210) arms `WaitThink` with `nextthink = time + 0.1`. So **0.1 s after
  a dropped/thrown/death-scattered weapon settles**, `WaitThink` fires and `UTIL_Remove`s it
  regardless of `m_flExpire` (which should keep it alive `weaponstay` = 5–60 s, or 30 s for throwables).
- Every player death in TS scatters the victim's weapons as `CTSWorldGun` ("WorldGun") entities. Under
  rcbot bot combat that is a **continuous high-rate create→settle→(0.1 s)→free** cycle → rapid
  recycling of edict slots.
- rcbot caches world-weapon/pickup **edict pointers** and re-scans entities (incl. via the engine's
  `pfnFindEntityByString`) each think. A WorldGun edict that rcbot latched as a pickup goal is freed
  almost immediately and its slot handed to the next created entity. When rcbot's cached edict now
  aliases a different/half-initialised occupant, or when the engine's linear scan touches a slot the
  DLL is mid-recycling, the read of `classname`/private data faults **inside** `pfnFindEntityByString`.
- **TS-specific** (WorldGun is a TS entity; stock valve never spawns it) and a **lifecycle edge** (needs
  the drop→settle→auto-free race to line up with rcbot's cache), which is consistent with "hard to
  reproduce even in 45 min."

### Exact repro condition (to fault)

rcbot bot(s) fighting on a TS map, weapons being dropped on death (or `drop`-thrown), so WorldGun
entities are continuously spawned and auto-removed 0.1 s after landing while rcbot is enumerating /
holding pickup edicts. The fault needs the freed WorldGun's edict slot to be reused (or scanned)
during the same window rcbot dereferences its cached copy.

### Proposed fix (mirror the original exactly)

```cpp
void CTSWorldGun::WaitThink()
{
    pev->nextthink = gpGlobals->time + 0.1f;
    if (m_flExpire < gpGlobals->time)
    {
        RemoveEntityFromSlowedArray(this);
        UTIL_Remove(this);
    }
}
```

This restores the original lifetime (dropped weapons persist for `weaponstay`) and eliminates the
premature-removal edict churn.

---

## Candidate #2 — latent `m_pSlowedEnts[]` raw-pointer dangle (same brace bug)

**Files:** `dlls/ts_worldgun.cpp:196`, `dlls/TS_util.cpp:126-148`, `dlls/player.h:410-416`

`RemoveEntityFromSlowedArray` scrubs a dying entity out of every player's **raw-pointer** array
`m_pSlowedEnts[32]` (player.h:415). The in-source warning is explicit (TS_util.cpp:126-130):
*"the array holds RAW pointers, so a missed scrub dangles."* Because of the #1 brace bug, the
non-expired removal path (`m_flExpire >= time`, the normal case) runs `UTIL_Remove` but **skips** the
scrub — so a WorldGun that a slow-motion player had captured into `m_pSlowedEnts` would be freed with
its raw pointer left behind.

**Why it's LOW priority *today*:** the slow-motion entity array is a deferred **SLICE**.
`CBasePlayer::SlowMotionFlow` (ts_player.cpp:304-305 SLICE note) only slows **players** (via a separate
`m_hSlowPlayers` **EHANDLE** array). Nothing currently **writes** `m_pSlowedEnts` (grep: only
`memset` @ ts_player.cpp:753 and the *read* in `RemoveEntityFromSlowedArray`), so `m_iSlowedEntCount`
stays 0 and the scrub is a no-op. The dangle is therefore **latent**: the moment the slow-motion
entity array is populated (the "TS_util freefunc group" / GoSlow-virtual work), this brace bug turns
into a **live cross-frame UAF** — a freed WorldGun's raw pointer sitting in `m_pSlowedEnts`, walked by
the slow-motion upkeep. Fixing #1 fixes this too; no separate change needed.

---

## Candidate #3 — `RespawnEntities` re-`Spawn()`s ts_powerup mid-loop (VERIFY, low)

**File:** `dlls/TS_gamerulesclass.cpp:358-360`

```cpp
if (lms)
    for (e = nullptr; (e = UTIL_FindEntityByClassname(e, "ts_powerup")) != nullptr;)
        e->Spawn(); // VERIFY: the original calls CTSPowerUp::Fall() (ts_powerup.cpp-local)
```

Calling full `Spawn()` on an already-linked map powerup re-runs `SET_MODEL`/`SetupPowerUp`/`ResetSequenceInfo`
on the **cursor** entity. It does **not** remove/free the cursor (no UAF), but it is a flagged behaviour
divergence and re-`SET_MODEL` mid-iteration is a smell. Only reachable in last-man-standing round
resets — **not** the reported repro (which excludes map/round churn). Low rank; recommend matching the
original (`Fall()`) for parity.

---

## What was audited and cleared

- **All TS `Find*` loops** (`TS_gamerulesclass.cpp:137/351-363/459/804/1032`, `TS_entities.cpp:516`,
  `ts_powerup.cpp:372/382`, `ts_player.cpp:197/1083`, `client.cpp:96/120/465`, `ts_grenades.cpp:247`,
  `TS_objectives.cpp:136/174`, `ts_briefcase` CaptureTouch player loop): **none remove the cursor and
  then engine-`->next` it**, and all removals are the deferred `UTIL_Remove`. Safe by construction.
- **Objective lifecycle** (`ts_flag` / `ts_briefcase` / `ts_bomb` / `ts_hack` / `ts_wingiver` /
  `ts_teamescape`): removals go through `Destroy`→`UTIL_Remove` (deferred) or `EnableObjective(false)`
  (no free). Cross-references use `EHANDLE m_flagStatus.hBriefcase` (safe). `CTSFlag::m_pOwner` is a raw
  `CBaseEntity*` but only ever a **player** (never freed at runtime).
- **`REMOVE_ENTITY` (immediate)** appears only at `TS_entities.cpp:575` (`CTSModel::Spawn`, no-model
  self-delete during map load — standard) and inside `CBaseEntity::SUB_Remove` (subs.cpp:117), which
  TS uses **only as a deferred think** (`SetThink(&...::SUB_Remove); nextthink += 0.1` @
  ts_worldgun.cpp:439). No inline `SUB_Remove()`/`REMOVE_ENTITY` on a Find-loop cursor.
- **Sibling slowed-array removal paths** (grenade/knife/shrapnel/flag): correctly braced (see #1 proof).
- **Runtime-created TS entities** all set `classname` before they can be iterated (see Background).

---

## Runtime steps to CONFIRM (for when the game can be driven)

1. **Reproduce cheaply first** (independent of rcbot): on any TS map, `drop` a weapon (or kill a bot to
   scatter one) and watch the dropped `WorldGun`. With the bug it **vanishes ~0.1 s after it settles**
   instead of lying there for `weaponstay` seconds. That alone confirms candidate #1 by observation.
2. **Correlate with the crash:** run metamod + rcbot with several bots, force continuous combat/deaths
   on a weapon-dense map. Set `weaponstay` high (e.g. `weaponstay 60`) to maximise the number of
   simultaneously-churning WorldGun edicts, or low (`weaponstay 5`) to maximise the free/reuse rate;
   either raises the collision odds with rcbot's cache.
3. **Instrument (optional):** log `entindex()`/`pev->classname` in `CTSWorldGun::WaitThink` at the
   `UTIL_Remove`; and log every `pfnFindEntityByString` result whose returned edict has `free==1` or a
   classname mismatch versus what rcbot cached. A fault coinciding with a just-freed WorldGun slot
   confirms the mechanism.
4. **Fix-and-verify:** apply the #1 brace fix; the dropped weapon should now persist for the full
   `weaponstay`, and the `FindEntityByString` fault should stop under the same bot load.

---

## Proposed fix summary

- **Apply candidate #1** verbatim (brace both calls under `if (m_flExpire < gpGlobals->time)`). This is
  the single confirmed removal-discipline divergence, restores original dropped-weapon lifetime, kills
  the premature-removal edict churn that feeds rcbot stale slots, and pre-empts the latent
  `m_pSlowedEnts` dangle (#2) once the slow-motion array lands.
- **Optionally** align candidate #3 (`ts_powerup` `Spawn()`→`Fall()`) for parity; not required for the
  crash.

If after the #1 fix the fault persists, the next place to look is **outside this repo** — rcbot's own
entity-pointer cache invalidation on `FL_KILLME`/edict-free for TS pickup classes (`WorldGun`,
`AmmoPack`, `ts_powerup`, `ts_flag`, `ts_groundweapon`) — since our removal discipline is otherwise
faithful to the original binary.
