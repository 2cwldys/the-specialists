# mod-ts change ledger

Running log of reconstruction progress. Newest entries on top. Keep factual:
what changed, how verified, what's next. See `CLAUDE.md` for mission/method.

---

## 2026-06-20 (cont. 3) — First reconstructed class: CTSSlowMotion (draft)

**Done**
- `dlls/ts_slowmotion.cpp` — first-draft reconstruction of `CTSSlowMotion`
  (+ behaviour of the helper `CTSSlowMotionPoint`, not yet split out).
- Recovered from the decompile (recovered/server/CTSSlowMotion.c) + FGD:
  - `CTSSlowMotion : public CBaseEntity`, brush trigger (`@SolidClass`).
  - Members (offset-annotated): m_iToggledOn(0x64), m_flStopTime(0x68),
    m_pPlayers[33](0x70), m_flRadius(0xf4), m_flDuration(0xf8 = "duration"),
    m_flSlowAmount(0xfc = "slowamt"), m_iAllMap(0x100 = SF "All map"),
    m_bSlowBullets(0x104 = SF "Slow bullets").
  - Methods: Spawn, KeyValue, Use, ActivateSlow, DeactivateSlow, WaitTouch,
    WaitThink, CheckPlayersDistance, Save/Restore (m_SaveData, 2 fields),
    ObjectCaps. LINK_ENTITY_TO_CLASS(ts_slowmotion, CTSSlowMotion).
  - Behaviour: slows players in range (per-player, tracks up to 33) OR globally
    via CTSGameRules::GlobalSlowMotion2 when "All map" set; duration>0 = timed,
    else toggle; fires pev->target + TSHudMessageAll on Use.

**NOT done / caveats (honest)**
- NOT compiled, NOT ABI-verified, NOT in dlls/CMakeLists.txt (explicit source
  list, so safe to leave out). Depends on un-reconstructed classes:
  CTSGameRules::GlobalSlowMotion2, TS player slow-motion virtuals
  (vtbl+0x14/0x18 — names VERIFY), TSHudMessageAll, FireTargets.
- VERIFY tags in the file: exact m_SaveData fields, m_flRadius source, the
  player slow-motion virtual names/indices, pev->message/target offsets,
  CheckPlayersDistance distance math (decompile was heavy stack arithmetic —
  reconstructed from intent).

**Next**
- Reconstruct CTSGameRules (esp. GlobalSlowMotion2) + the TS player slow-motion
  virtuals so CTSSlowMotion's deps resolve, then compile + ABI-verify.

---

## 2026-06-20 (cont. 2) — GNU-v2 demangler + prototype-annotated decompiles

- `scripts/gnv2demangle.py`: gnu-v2 demangler (c++filt dropped it). 1788/2426
  (74%) typed signatures -> `reference/signatures-gcc2.{map,txt}`, incl. the TS
  classes mp.dll never exported. Handles member/global/ctor/dtor, P/R/C/U/G,
  N repeat-args, 0-based T backrefs.
- `DecompExport.java` annotates each decompiled function with its recovered
  prototype; `decompile.sh` feeds the map automatically.

## 2026-06-20 (cont.) — Authorization + Ghidra pipeline

- Authorization: author **Vino ("Vinoisnotozou")** OK'd reconstruction; original
  source lost. (reference/README.md)
- Ghidra headless pipeline: `scripts/ghidra/DecompExport.java` +
  `scripts/decompile.sh`. Smoke test CTSSlowMotion: 28 funcs, 0 failures.

## 2026-06-20 — Scaffolding, reference, tooling, work-list

- Private `goldsrc-net/mod-ts`, fork of `halflife-updated@64bit-demo`, at
  `~/Projects/goldsrc.net/mod-ts`. Standalone `ts/` game dir.
- `reference/`: ts_i386.so, mp.dll, client.dll, ts_fgd.fgd + dump-signatures.sh.
- Build: clean separation; `scripts/build.sh <arch>` via hlds64-buildchain image
  (NOT a buildchain submodule — reverted).
- Tooling: Ghidra 12.1.2 + rizin rz-* on PATH.
- Work-list `reference/RECONSTRUCTION.md`: 60 TS-specific classes (~386 funcs),
  23 FGD-backed. CTSGun(44), CTSGameRules(41), CTSStunt(25).

**Conventions:** NO `Co-Authored-By` trailer. CLAUDE.md local-only. Auto mode:
halt & ask on a block.
