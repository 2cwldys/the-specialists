# CTSKnife::WaitTouch — analysis

Original binary: `reference/ts_i386.so`, nm addr **0x6dce8** (Ghidra 0x7dce8), size 185 bytes.
Original source file (filemap): **TS_grenades.cpp** (CTSKnife lives with the throwables).

## 1. SIGNATURE

From `reference/signatures-gcc2.txt` (line 1169) + the typed decompile:

```
void CTSKnife::WaitTouch(CBaseEntity *pOther)      // WaitTouch__8CTSKnifeP11CBaseEntity @ 0x6dce8
```

Installed as the entity **touch** handler (`SetTouch`), replacing `BounceTouch`, by
BounceTouch's settle tail (see CALLERS). Note the `pOther` argument is **completely
ignored** — the function never reads it.

## 2. BEHAVIOR

WaitTouch is the *post-first-contact* touch handler: pure ground-settle velocity damping.
No damage, no pickup, no sound, no state-machine transitions, no messages/events.

Step by step (decompile `melee_waittouch.c`):

1. **Gate:** `if (pev->flags & FL_ONGROUND /*0x200*/)` — only acts when the engine has
   flagged the knife on-ground at this touch. Any airborne touch is a no-op (the knife is
   still MOVETYPE_BOUNCE; the engine's bounce physics handles the actual deflection).
2. **Damp:** `pev->velocity = pev->velocity * 0.5f` (constant `fc_0_5` = 0.5, all three
   components). Each ground contact halves the speed — this is the "friction" that makes
   the bounced knife come to rest.
3. **Rest test:** if `|pev->velocity| < 32.0f` (constant `fc_32` = 32, computed AFTER the
   halving):
   - `pev->angles.x = 0; pev->angles.z = 0;` — zero pitch and roll → the knife **lies flat**
     (yaw preserved).
   - `pev->velocity = g_vecZero` (copied from the `g_vecZero` global via GOT
     `PTR_g_vecZero_00177698`) — full stop.
4. Return. Nothing else: no `SetTouch(NULL)`, no movetype/solid change, no glow, no
   removal, no re-arm of think.

Members read/written (all `pev->`; the knife's own C++ members are untouched here):
| offset (entvars) | field | use |
|---|---|---|
| flags (& 0x200) | FL_ONGROUND | the only gate |
| velocity (0x20) | read+write | x0.5 per ground touch; zeroed at rest |
| angles (0x50) | write .x/.z | flatten (pitch=0, roll=0) at rest |

Constants: **0.5** (per-ground-touch damping factor), **32.0** (rest speed threshold,
units/s), `g_vecZero`.

Callees: none (no engine calls, no virtuals, no PLT calls — leaf function apart from the
`g_vecZero` GOT load).

### Context: the surrounding state machine (what installs it, verified by disassembly)

`nm`-confirmed method set: Spawn 0x6db54 · BounceTouch 0x6decc · **WaitTouch 0x6dce8** ·
WaitThink 0x6ea30 · Use 0x6dda4 · GetDescription 0x6db04 · ShouldCollide 0x6ed54 ·
ObjectCaps 0x6f04c (W) · CreateKnife 0x6d9e4.

The **only** code reference to WaitTouch in the whole binary is its GOT slot
(`readelf -r`: R_386_GLOB_DAT @ GOT+0x1fd0) loaded at **0x6e9b5 inside
`CTSKnife::BounceTouch`** — the common settle tail (nm 0x6e976–0x6ea23 = Ghidra
LAB_0007e985 region). That tail does, in order:

- (if `UTIL_PointContents(pev->origin) != CONTENTS_SKY (-3)`) `UTIL_Sparks(pev->origin)`;
- `EMIT_SOUND_DYN(pev->pContainingEntity /*+0x208*/, chan=4 /*CHAN_BODY*/,
  "weapons/knife/knife_hit.wav", vol=1.0, attn=0.8 /*ATTN_NORM*/, flags=0, pitch=100 /*0x64*/,
  0.5f)` — NB the TS build's `EMIT_SOUND_DYN` has an extra trailing float
  (gcc2 sig: `EMIT_SOUND_DYN(edict_s*, int, const char*, float, float, int, int, float)`);
  the tail passes **0.5** there. VERIFY what the trailing float means (likely TS's
  slow-motion pitch handling is inside this wrapper, not at the call sites).
- **`SetTouch(&CTSKnife::WaitTouch)`** — gcc2 PMF store: `this+0x14 = 0xffff0000`,
  `this+0x18 = &WaitTouch` (m_pfnTouch);
- **`SetThink` re-armed to the *virtual* `CTSKnife::WaitThink`** — PMF `{index=0x46=70,
  voffset=0x60}` -> vptr@0x60, vtable byte 0x114 = slot 69 = `CTSKnife::WaitThink()`
  (confirmed in `reference/coverage/VTABLES.md`). So the swept-break/expire think keeps
  running after settle.
- `this+0x90 = 0` (byte) — clears the **in-flight flag** (set to 1 in Spawn @0x6dcbe;
  `CTSKnife::Use` refuses pickup while it's nonzero — see below);
- `pev->nextthink (0x104) = gpGlobals->time + 0.1` (double @0xf7138);
- `pev->gravity (0x11c) = 0.75f` (0x3f400000);
- `this+0x88 (float, m_flExpire) = gpGlobals->time + 10.0` (double @0xf7140) — the settled
  knife is auto-removed by WaitThink ~10 s later if not picked up (Spawn set it to +30 s).

Note what the tail does **not** do: movetype stays MOVETYPE_BOUNCE, solid stays SOLID_BBOX,
no glow/renderfx, no trigger conversion. The settled knife stays a physical bouncing point
object whose touch handler is now the damping WaitTouch; pickup is exclusively the
**+USE path** (`ObjectCaps`=FCAP_CONTINUOUS_USE=0x10 -> `CTSKnife::Use`).

### Bonus: `CTSKnife::Use` @0x6dda4 fully decoded (the pickup WaitTouch feeds into)

Because our Use is missing gates, decoded here from disassembly:
1. `if (this+0x90 != 0) return;` — no pickup while in flight (flag cleared only by the
   BounceTouch settle tail / early clear @0x6df64).
2. `if (!pActivator->IsPlayer() /*vt 0xc8*/) return;` and
   `if (!pActivator->IsAlive() /*vt 0xb4*/) return;`
3. `if (pActivator->pev->effects & 0x80 /*EF_NODRAW, sign-bit test on byte @pev+0x118*/) return;`
4. `if (this+0x91 /*byte*/ && !KungFu(pActivator)) return;` — **+0x91 = "is katana" flag;
   a thrown katana IS pickable, but only by a player for whom the global `KungFu()` returns
   true.** (Contradicts our current comment "katana not recoverable, user confirmed" — VERIFY
   in-game, but the binary is unambiguous that a PickUpWeapon(34) path exists.)
5. `if (this+0x88 <= gpGlobals->time)` -> skip PickUpWeapon (treat as success) -> remove:
   an expired knife is silently cleaned up on use.
6. Else dispatch on `this+0xa0` (int, internal knife-type code):
   `==8` -> `PickUpWeapon(player, 34 /*TSW_KATANA*/, NULL, 0, false, false)`;
   `==6` -> `PickUpWeapon(player, 25 /*TSW_KNIFE*/, ...)`;
   else  -> `PickUpWeapon(player, 35 /*TSW_SEALKNIFE*/, ...)`
   (gcc2 sig `CBasePlayer::PickUpWeapon(tsweapon_t, void*, int, bool, bool)`).
7. If PickUpWeapon returned nonzero (or the expire shortcut hit): refresh `this+0x88`,
   `RemoveEntityFromSlowedArray(this)` (slow-motion bookkeeping), `UTIL_Remove(this)`.
   If PickUpWeapon returned 0 (inventory full etc.): the knife stays.

CTSKnife transient members recovered along the way (cross-checked `members_auto.tsv`):
| offset | type | meaning |
|---|---|---|
| +0x88 | float | expire/settle timer (Spawn: time+30; settle tail: time+10; Use gate) |
| +0x90 | byte | in-flight flag (Spawn=1; BounceTouch clears @0x6df64 and in the tail; Use gate) |
| +0x91 | byte | katana flag -> KungFu-gated pickup |
| +0x94/0x98/0x9c | int | present in members_auto — VERIFY meaning (likely damage/owner/aux) |
| +0xa0 | int | knife-type code: 8=katana, 6=knife, other=sealknife (NOT the tsweapon_t id) |

## 3. CALLERS

- **No direct callers anywhere in the binary.** The single reference is the GOT-slot load
  at 0x6e9b5 storing it into `m_pfnTouch` — i.e. it is only ever invoked by the **engine's
  touch dispatch** after installation.
- Installed exclusively by **`CTSKnife::BounceTouch`'s common settle tail** (0x6e976+),
  reached by: the **player-stab** branch (`jmp 0x6e254 -> 0x6e985`), the **glancing-slide**
  branch (`jmp 0x6e4f9 -> 0x6e948`), and the **second stick variant** (`0x6e8ae -> 0x6e941`,
  which copies a 3-vector then falls into the tail). Paths that exit WITHOUT installing it:
  the owner/EF_NODRAW guards (-> epilogue 0x6ea24), the path ending `jmp 0x6e709` (one of the
  worldspawn embed variants), and the ignore-other-"knife" branch (0x6e8dc). VERIFY the exact
  branch<->gameplay mapping against the existing BounceTouch RE in `dlls/ts_knife.cpp` — for
  this analysis what matters is: **stabbed-and-bounced and slid knives get WaitTouch;
  at least one embed path does not.**
- `melee_refs.txt` (the provided refs dump) covers the CBasePlayer melee methods
  (UpdateMeleeBar/CloseCombatAttack/ColdCockAttack/BlockPlayer ...) and contains **no**
  reference to CTSKnife::WaitTouch — consistent with the GOT analysis (engine-dispatched
  only). Sibling classes (CTSWorldGun, CTSPowerUp, CTSFlag, ...) each have their own
  unrelated `WaitTouch` (it's the TS naming convention for pickup/idle touch handlers).

## 4. OURS

`dlls/ts_knife.cpp` — the equivalent role is spread across **invented** methods:

- **`CTSKnife::StartSettle()`** (ts_knife.cpp:254) + **`CTSKnife::SettleThink()`** (:263)
  = the closest equivalent of the settle tail + WaitTouch. Deliberately divergent (the
  comment at :251 says so): ours switches to **MOVETYPE_TOSS**, clears the touch handler
  (`SetTouch(nullptr)`), and **polls FL_ONGROUND in a think**; the original keeps
  MOVETYPE_BOUNCE and does it **touch-driven**. Ours has **no per-touch 0.5 velocity
  damping and no <32 u/s rest threshold**; SettleThink does flatten (`angles.x/z = 0`,
  matches) and zeroes avelocity (original WaitTouch does NOT touch avelocity — it zeroes
  **velocity**; VERIFY avelocity: the original probably relies on the engine or leaves it).
  StartSettle's `gravity = 0.75` and BounceTouch's `m_flExpire = time+10` DO match the
  original tail.
- **`CTSKnife::BecomePickup()`** (:123) — pure invention: glow shell, SOLID_TRIGGER,
  30 s linger, katana-becomes-inert. The original has NO pickup-conversion at all; a settled
  knife stays a plain SOLID_BBOX bouncer pickable via +USE, and expires ~10 s after settling.
- **`CTSKnife::PickupTouch()`** (:279) — invention (touch+IN_USE pickup). Original pickup
  is only `Use` (FCAP_CONTINUOUS_USE); our `ObjectCaps` already returns FCAP_CONTINUOUS_USE
  and we DO have a `Use` (:318), but ours is missing the original's gates: no in-flight
  (+0x90) gate, no expire gate, no **katana->KungFu** gate, no `RemoveEntityFromSlowedArray`,
  and it doesn't handle PickUpWeapon failure (inventory full -> knife should stay).
- **`CTSKnife::StickAt()`** (:168) — a factored-out fragment of the original's BounceTouch
  worldspawn-embed path (inline in the original, not a separate function).
- `WaitThink` (:91) and `BounceTouch` (:193) exist under their **original names** — good —
  but BounceTouch's settle handoff calls the invented StartSettle instead of installing
  WaitTouch, and does not re-arm the think/nextthink/in-flight flag the way the tail does.
- Member diffs: we have `m_flExpire` (+0x88 provenance ok) and `m_knifeType` — but ours
  stores the `TSW_*` id directly while the original's +0xa0 stores the internal 6/8/x code
  plus a separate +0x91 katana bool and +0x90 in-flight bool (audit-relevant member gaps).
  Original `CreateKnife` signature is `(CBasePlayer*, Vector, Vector, int)`
  (`CreateKnife__8CTSKnifeP11CBasePlayerG6VectorT2i`) — ours takes
  `(pOwner, type, damage, vecStart, vecVel)` (extra damage param, different order).

**Verdict: exists-partial** — the settle role exists as SettleThink/StartSettle but under
invented names with a different mechanism and missing the damping formula; the pickup side
(BecomePickup/PickupTouch) is invented and replaces the original WaitTouch+Use contract.

## 5. RECONCILE

Server-side only — **no client<->server protocol impact** (WaitTouch emits no user messages,
no events, no sounds itself; the tail's knife_hit.wav is server EMIT_SOUND_DYN, already
present in our BounceTouch, though ours uses CHAN_WEAPON + TSSlowPitch where the tail uses
CHAN_BODY + pitch 100 + trailing 0.5 — VERIFY channel/pitch per branch when reconciling
BounceTouch). Plan:

1. **Write `CTSKnife::WaitTouch(CBaseEntity*)` fresh** (it's ~10 lines): FL_ONGROUND gate ->
   `velocity *= 0.5` -> if `velocity.Length() < 32` -> `angles.x = angles.z = 0`,
   `velocity = g_vecZero`. Keep MOVETYPE_BOUNCE (do NOT switch to TOSS).
2. **Rewire BounceTouch's settle tail** to match the original exactly: replace the
   `StartSettle()` calls with `SetTouch(&CTSKnife::WaitTouch); SetThink(&CTSKnife::WaitThink);
   pev->nextthink = time + 0.1; pev->gravity = 0.75; m_bInFlight = false;
   m_flExpire = time + 10;` (sparks + hit sound already in the tail region per branch).
3. **Delete the inventions this absorbs:** `StartSettle`, `SettleThink`, `BecomePickup`,
   `PickupTouch` (four methods gone -> audit INVENTED count drops). `StickAt` can stay as a
   local helper OR be re-inlined into BounceTouch (the original has no such function —
   audit-strict would inline it; at minimum mark it as a non-original refactor).
4. **Fix `Use` to the original's gates:** add the in-flight flag (`m_bInFlight`, orig
   +0x90; set true in Spawn, cleared in the settle tail and at BounceTouch 0x6df64 — VERIFY
   which early branch that is), the expire check (expired -> remove without giving), the
   **katana KungFu gate** (needs a `KungFu(CBasePlayer*)` helper — we have `m_iKungFu` on
   the player but no such global; VERIFY the original KungFu() semantics before wiring),
   keep-knife-on-PickUpWeapon-failure, and slowed-array removal (VERIFY whether our
   slow-motion implementation tracks a slowed-entity array — `RemoveEntityFromSlowedArray`
   exists in the original; we have slowmo code but confirm the array equivalent).
5. **Katana behaviour change:** per the binary, a thrown katana is +USE-recoverable by
   KungFu players — replaces our "katana becomes inert decoration" branch in BecomePickup
   (which is being deleted anyway). VERIFY in the live original (hl.exe is available) since
   our comment claims the user observed the opposite; the Use disassembly is unambiguous
   that the PickUpWeapon(TSW_KATANA=34) path exists, gated on KungFu().
6. **Member parity (audit.py):** add `m_bInFlight` (+0x90) and the katana bool (+0x91);
   consider storing the original's internal type code (+0xa0: 6/8/x) or document the
   TSW-id mapping as a provenance note. Curate these into `members_re.tsv`
   (CTSKnife 0x90/0x91/0xa0) once implemented. +0x94/0x98/0x9c remain UNRESOLVED — VERIFY
   via `re.sh CTSKnife` when the Ghidra project is free.
7. **Gameplay consequence to accept:** the original removes a settled knife ~10 s after it
   comes to rest (WaitThink expire) with no glow — our 30 s glowing pickup is a divergence
   the drop-in bar says we must drop (or gate behind a non-default cvar if the user wants
   to keep the nicety).
