# cont.83 RE working set (typed decompiles + extractions produced this session)

All from `reference/ts_i386.so` via `dc.sh server 0x<nm-addr>` (Ghidra addr = nm + 0x10000)
unless noted; regenerate any missing piece the same way. The `re` project was current all
session (no `re.sh setup` needed).

## Server decompiles (dc.sh)
- `six_missing.c` — the six then-missing methods in one batch: EmptySound @0x71be4 ·
  StopReload @0x71288 · ReloadWeapon @0x74a14 · PickUpWeapon @0x8314c · ShootBulletsPlayer
  @0x5f69c (gcc2 struct-return: the FIRST decompile param is the sret Vector, `_a0` = this) ·
  TSInit @0x92a4c(ghidra). ALL APPLIED.
- `idle_shotgunreload.c` — WeaponIdle @0x754dc + ShotgunReload @0x74aac (the pump machine +
  burst machine + auto-reload truth; reload.sc fparam1 8/9/10). APPLIED.
- `fire_coldcock_mult.c` — Fire @0x71c78 (waterlevel click, akimbo 8/9 pose vs pump TSA_FIRE) ·
  ColdCock @0x71dd0 (GrenadeIdle(1)+StopReload, four timers, coldcock.sc gun+0x966) ·
  MultiplyClips @0x71308 (the STACKABLE slot-funded branch). APPLIED.
- `ccombat.c` — CCombatBlock @0x75ebc (four-gate head; closecombat.sc iparam2 1/0x12) +
  CCombatAttack @0x76078 (the per-move event args came from DISASM SLOT-TRACKING, not this
  decompile — its local_108 merged two stack slots; the FLY family's animIdx is still VERIFY).
  APPLIED except the fly-family index.
- `takeweapon.c` — TakeWeapon @0x70a04: NO ammo grant; arg3 = ATTACH MASK; the +1 stack merge
  capped at iPellets; single-refused-when-akimbo-owned; attach/active unions. APPLIED.
- `buyweapon.c` — BuyWeapon @0x845bc: the attach-bit prices (sil 900/laser 800/flash 500/scope
  0x708); PickUpWeapon(type,0,mask,1,1). APPLIED. (BuyAmmo @0x841e8 decompiled inline in the
  session: the FILL-TO-CAP mag loop — APPLIED.)
- `akimbofire.c` — AkimboFire @0x729c0: the user-model VERIFY RE (free modes per-barrel with
  dry EmptySound; both-buttons=LINKED — mode 5 held / mode 4 the pressed cache player+0x814;
  ScaleSpread 4th arg = SILENCED; recoil x2 linked; sil dmg x0.775; the ft-4 single mirror).
  APPLIED.
- `standardfire.c` — StandardFire @0x724a4: recoil x5 when moving >175 u/s; the sil x0.775
  damage + the SILENCED ScaleSpread arg (both call sites). APPLIED.

## Client dossier (the gun-tail subagent; clientdll project)
- `dossier-ev-guntail.md` + `ev_empty/ev_reload/ev_pump/ev_grenade/ev_firelinked/ev_fire_core/
  ev_fire_helpers/ev_fire_wrapper/ev_guntail_all.c` — the five handler contracts. APPLIED —
  with TWO label corrections proven later by the MODELS: the anim-block sides ([22/23]=RIGHT,
  [24/25]=LEFT — the dossier had them swapped) and the akimbo attachment pairs (0/1 right,
  2/3 left, from the embedded 5001/5011 vs 5021/5031 flash events).
- `animblocks.md` — the full 29-short anim blocks (movl-PAIR-aware!), flReloadTime(+0x14),
  iStackable(+0xdc), the coldcock [28] column, the reload.sc wire contract.

## Extraction techniques proven this session (reuse them)
- **dc.sh batch decompiles** (multiple addrs per run) before ANY reconstruction.
- **Disasm caller/arg decode**: python push-sequence reads at call sites (resolved every
  PickUpWeapon/SetPlayerRespawnTime/closecombat-arg contract) — cheap and definitive when the
  decompiler garbles stack slots.
- **Write_ column extraction** with the movl-PAIR fix (a movl at a short-array offset writes
  TWO shorts). ⚠ SPARTA: never extract +0x1c/+0x28/+0x30/+0x34 from Write_ bodies.
- **MODEL dumps settle presentation disputes**: sequence names (the akimbo left/right sets, the
  hold change/rechange), EMBEDDED SEQUENCE EVENTS (5001-family = who really draws the flash),
  bone names (the ITALIAN-bone gait split), attachment pairings. python struct reads of the
  .mdl headers — see the ledger round 4/6c/6d entries for the exact offsets.
- **Loose lea-pairing BLEEDS** — the order-agnostic string-copy extractor produced false pairs;
  adjudicate by strict-run agreement or hand-reading the disasm around each store.
