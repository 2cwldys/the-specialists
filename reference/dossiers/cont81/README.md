# cont.81 RE working set (dossiers + typed decompiles)

Preserved from the cont.81 session scratchpad so later sessions don't re-decompile.
All content derives from `reference/ts_i386.so` / `mp.dll` via the typed Ghidra `re`
project (`scripts/ghidra/re.sh`) — regenerate any missing piece the same way.

- `dossier-melee.md` (+`dossier-melee-addendum-L1-L2.md`) — the 9-missing-method melee
  cluster: system map, per-method facts, the S0-S10 reconcile order (S0-S6+S9 APPLIED;
  S7a/b/c + S8 + S10 REMAIN), the closed L1/L2 gates. Depth: `analysis_<method>.md` +
  `melee_<slug>.c` decompiles + `melee_refs.txt` callers.
- `dossier-gmsg.md` + `star_*.md` — the 14 user-message protocol plan U0-U5 (NOT yet
  applied except the TSFlag wire-name fix); star_LinkUserMessages.md has the full
  64-entry registration order. Depth: `analysis_gmsg_<name>.md`.
- `dossier-freefuncs.md` — all 43 missing free functions RE'd, plan F1-F10 (NOT yet
  applied). Depth: `analysis_ff_<group>.md` + `ff_<func>.c`.
- `plan-consolidation.md` + `plan_files_*.md` — the file-layout master plan (C1-C20:
  FULLY APPLIED).
- `dossier-ctsstunt.md`, `CTSStunt_all.txt`, `animate-decomp.txt`, `ctsanim-ctor.txt`,
  `gowheel/goroll/jump/Duck/GetDirection` — the cont.80/81 stunt+animation ground truth.
- `pm_*.c`, `setanim.c`, `blend_helpers.c`, `dropweapon.c`, `holster.c`, `deploy.c`,
  `speed.c`, `mp_animate.c` — the movement/anim decompiles behind the cont.81 physics
  and pose fixes.
