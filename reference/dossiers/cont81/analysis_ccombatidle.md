# CTSGun::CCombatIdle — analysis (ts_i386.so @ nm 0x75e54 / Ghidra 0x85e54, size 104)

## 1. SIGNATURE

From `reference/signatures-gcc2.txt` (line 1117) and the mangled symbol `CCombatIdle__6CTSGun`:

```cpp
void CTSGun::CCombatIdle();            // gcc2: CCombatIdle__6CTSGun, no args, void return
```

Non-virtual (a plain member function — it does NOT appear in any vtable in
`reference/coverage/VTABLES.md`; it is called direct through the PLT from
`WeaponIdle__6CTSGun`).

## 2. BEHAVIOR

This is the **kung-fu (bare hands) viewmodel idle re-player** — tiny and pure-cosmetic.
Despite the "close-combat" name and the `closecombat.sc`/`closecombat.wav` hints, it plays **no
event, no sound, does no damage, reads no buttons, and touches no iuser bits**. Verified against
the raw disassembly (objdump @0x75e54–0x75ebb):

Step by step:

1. `if (UTIL_WeaponTimeBase() < m_flTimeWeaponIdle) return;`
   - member read: `this+0xa8` = **`CBasePlayerWeapon::m_flTimeWeaponIdle`** (float; save-table
     confirmed, `members.tsv:80`). The decompile's `field_0xa8` is this inherited member (the
     auto skeleton typed it `int` from the raw store — it is the standard SDK float idle timer).
2. `m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;`
   - constant: `dc_3` = the double **3.0** (`fldl` of a .rodata double) — the kung-fu idle
     cadence is one idle anim every 3 seconds.
3. `SendWeaponAnim( (int)*(short*)(get_weapon_info(0) + 0x252), 0, 0 );`
   - `get_weapon_info__F10tsweapon_t(0)` — literal **0 = TSW_MELEE** (confirmed in the disasm:
     `push $0x0` before the PLT call; NOT `m_weaponType`). Returns `&s_weapon_info[0]`, the
     kung-fu table entry.
   - `wi+0x252` is the first `short` of the weapon's **normal-stance anim block** (the block
     `TakeWeapon` points `item+0x28` at, vs the hold-stance block at `wi+0x28c`) = the **idle
     viewmodel sequence index**. Every `Write_<weapon>` in the binary stores `movw $0x0` to
     `+0x252`, so for kung fu this evaluates to **sequence 0** of `models/v_melee.mdl` — but
     source-faithfully it is a table read, not a hardcoded 0.
   - Dispatch: **virtual** through `this->vtbl` (vptr @ `this+0x60`, gcc2 layout) at slot offset
     **0x174** = `CBasePlayerWeapon::SendWeaponAnim(int, int, int)` (VTABLES.md, CTSGun vtable),
     called as `SendWeaponAnim(this, anim, 0, 0)` — i.e. `iAnim`, `skiplocal=0`, `body=0`.

That is the whole function. No other members read or written; no gmsg*, no PLAYBACK_EVENT, no
EMIT_SOUND, no timers other than `m_flTimeWeaponIdle`.

## 3. CALLERS

`melee_refs.txt` contains **no** references to CCombatIdle (it is not part of the attack/damage
chain). Direct binary xref scan (objdump, `call CCombatIdle__6CTSGun@plt`) finds exactly **one
call site**: `WeaponIdle__6CTSGun` @0x754dc, call at **0x754fe** — the FIRST branch of
WeaponIdle:

```
754f1:  cmpl $0x0, 0xcc(%esi)      ; this->m_pActiveItem (CTSGun+0xcc, s_weapon_status*)
754f8:  jne  <armed path>
754fe:  call CCombatIdle           ; then jmp to function epilogue (return)
```

i.e. the original's `CTSGun::WeaponIdle()` begins with:

```cpp
if (m_pActiveItem == NULL) { CCombatIdle(); return; }   // NULL = bare hands / kung fu
```

(`0xcc` = `m_pActiveItem`, `members_re.tsv`.) The armed path then refreshes `this+0xb4` from
`item+4`, re-points the active anim block (`item+0x28`) at `wi+0x252` or `wi+0x28c` per the hold
flag `item+0x2c`, etc. `WeaponIdle` itself is the standard virtual dispatched from
`CBasePlayerWeapon::ItemPostFrame` when no attack/reload buttons are down — so the run-time
condition for CCombatIdle is: **kung fu wielded, no ATTACK/ATTACK2/RELOAD activity, and (its own
internal gate) the idle timer elapsed**.

## 4. OURS

**Absent.** There is no `CCombatIdle` anywhere in our tree, and no kung-fu idle-anim replay:

- `dlls/ts_gun.cpp:2077` — our `CTSGun::WeaponIdle()` (correctly tagged `@ WeaponIdle__6CTSGun
  (@0x854dc)`) handles ONLY burst continuation (`m_fireMode == TSFM_BURST && m_burstCount > 0`
  → `Fire()`) + `ResetEmptySound()`. The original's leading `if (!m_pActiveItem) { CCombatIdle();
  return; }` branch is missing entirely.
- `dlls/ts_gun.cpp:1957–1958` — our ItemPostFrame-side driver already gates
  `WeaponIdle()` on `m_flTimeWeaponIdle <= UTIL_WeaponTimeBase()`. NOTE: the original gates
  inside CCombatIdle itself (WeaponIdle is invoked un-gated for the melee branch); our outer
  gate is equivalent for this function's behaviour (double gate is harmless), but see RECONCILE.
- `dlls/ts_gun.h:128` — we DO already have `TSItem *m_pActiveItem` (`+0xcc`, NULL = kung-fu),
  so the original's dispatch gate maps 1:1.
- `dlls/ts_gun.h:93–94` — we have the siblings `CCombatAttack` / `CCombatBlock` but not
  `CCombatIdle`. Our `CCombatBlock` (ts_gun.cpp:1276) already uses the same `+3.0` idle re-arm,
  consistent with this function's cadence.
- `dlls/ts_weapon_info.h` — our `s_weapon_info` reconstruction has `iFireAnim`/`iReloadAnim`/
  `iHold*` but **no idle-anim field** (no representation of the normal anim block's first short
  at `wi+0x252`). The kung-fu idle sequence therefore has no home in our table yet.

Net effect of the gap: with bare hands out, our viewmodel never re-plays its idle sequence every
3 s (and our WeaponIdle's burst logic runs even when unarmed, though it no-ops).

## 5. RECONCILE

Nothing to rename — this is a **write-fresh** (it absorbs none of our invented methods; our
MeleeAttack/ThrowKnife/etc. are unrelated to this function):

1. **Add `void CCombatIdle();`** to `CTSGun` (ts_gun.h, next to CCombatAttack/CCombatBlock, with
   the `@ CCombatIdle__6CTSGun (@0x85e54)` provenance tag). Body, exactly:
   ```cpp
   if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase()) return;
   m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
   SendWeaponAnim(<kung-fu normal idle seq>);   // original: *(short*)(get_weapon_info(TSW_MELEE)+0x252), == 0
   ```
2. **Re-wire our `CTSGun::WeaponIdle()`** (ts_gun.cpp:2077) to lead with the original's branch:
   `if (!m_pActiveItem) { CCombatIdle(); return; }` before the burst logic. This also stops the
   burst path from running unarmed, matching the original's structure.
3. **Idle-anim data**: add the normal-set idle sequence to `s_weapon_info` (e.g. `iIdleAnim`,
   provenance `+0x252`), initialized 0 for all weapons (every `Write_<weapon>` stores 0), and
   read it in CCombatIdle instead of hardcoding — keeps the table shape converging on the
   original's anim-block layout that `TakeWeapon`/`SecondaryAttack`/`Reload` also index.
4. **Protocol**: none. `SendWeaponAnim` is the stock SVC_WEAPONANIM path — no user message, no
   event, nothing byte-sensitive. `closecombat.sc` is NOT involved here (it belongs to the
   attack path, not idle).
5. **VERIFY (flagged, not guessed):**
   - VERIFY: CTSGun is client-predicted; whether the client.dll's shared weapon code runs the
     same CCombatIdle client-side (so the idle anim is predicted rather than server-sent). If
     so, the body must live in the shared/predicted path on both libs like the other CTSGun
     timers (per the predicted-weapon gotchas memory).
   - VERIFY: our outer `m_flTimeWeaponIdle` gate at ts_gun.cpp:1957 vs the original's un-gated
     WeaponIdle dispatch — check the original CBasePlayerWeapon::ItemPostFrame/our driver so the
     armed-path WeaponIdle work (anim-block re-point etc., a separate parity gap) isn't starved
     by our extra gate.
   - VERIFY at runtime: `v_melee.mdl` sequence 0 is the idle (expected — Write_ stores 0 —
     confirm visually once wired).
   - Note: the original WeaponIdle's armed path (item anim-block repointing, wi+0x348 check,
     item+2 mode checks, ~0x18d bytes) is larger than our burst-only version — a separate
     follow-up target, out of scope for CCombatIdle itself.
