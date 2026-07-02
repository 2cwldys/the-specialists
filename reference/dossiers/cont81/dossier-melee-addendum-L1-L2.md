# dossier-melee ADDENDUM — live-trace gates L1 + L2 CLOSED STATICALLY (cont.81)

Method: lock-free objdump scan for every `0x774(`/`0x7a8(`/`0x7ac(`/`0x7b0(` access in
`reference/ts_i386.so`, nearest-symbol resolution, .rodata constant reads via GOT base
0x165E10. No Ghidra, no live trace — asm is ground truth.

## L1 CLOSED — melee-bar (player+0x774) regen site + rate

The regen does NOT go through UpdateMeleeBar (that's why refs found no negative-amount
caller). It is a direct member update in **CBasePlayer::PreThink** (@0x808cc, block at
+0x4eb..+0x547 = 0x80db7..0x80e13):

```
// earlier in PreThink (+0xf5..+0x126):
local_2c = GiveSlowMul() * gpGlobals->frametime      // vtbl slot 0x10 = GiveSlowMul(); GOT+0x1600 -> gpGlobals, +4 = frametime
local_1c = (m_iPowerups & 0x10 /*TS_PWUP_DBLFIRE*/) ? local_2c * 1.5f : local_2c
// local_1c is THE per-frame time delta used by ALL the PreThink timer updates below.

// the regen block (+0x4eb):
if (m_flMeleeBar < 100.0f) {                          // C1 @0xf761c = 100.0f; skip when full
    if (m_flMeleeBar >= -0.001f)                      // C2 @0xf6a0c = -0.001f (normal path)
        m_flMeleeBar += local_1c * 15.0f;             // C3 @0xf7f84 = 15.0f  => 15 points/sec
    else
        m_flMeleeBar += local_1c;                     // negative-bar sentinel path (transcribe faithfully)
    if (m_flMeleeBar >= 100.0f) m_flMeleeBar = 100.0f; // clamp (movl $0x42c80000)
}
```

- **Rate = 15.0/sec** (slow-mo-scaled via GivelowMul, ×1.5 under TS_PWUP_DBLFIRE). Our
  invented RegenBlockBar's 25/sec was user-tuned guesswork — replace.
- **Init to 100.0** in `CBasePlayer::TSInit` (@0x82a4c +0x357, movl $0x42c80000) — TSInit
  is one of the 19 missing methods; this is one of its duties.
- Note: NO gmsgKFuPower send from the regen path (UpdateMeleeBar sends per-call; the regen
  is silent — the client-side bar presumably predicts/interpolates or rides WeaponInfo).
  VERIFY how the client learns the regenerated value (WeaponInfo melee clip byte per the
  UpdateClientData read @0x75aca).

## L2 CLOSED — block-window members (player+0x7a8/0x7ac/0x7b0) decay mechanism

Stored as a VALUE (0.5), decayed per-frame in **CBasePlayer::PreThink** (+0x1ce..+0x22d =
0x80a9a..0x80af9), one block per window, pattern:

```
window -= local_1c;                 // same slow-mo/dblfire-scaled frame delta
if (window < 0.0f) window = 0.0f;   // floor at 0 (fcom vs C4 @0xf6a08 = 0.0f)
```

All three windows (0x7a8 kungfu?, 0x7ac, 0x7b0 — per-weapon assignment per the Victim*Hold
readers) get the same treatment. Immediately after (+0x237): a FOURTH timer at
**player+0x7b4** decays FASTER: `t -= local_1c * 5.0f` (mult @0xf7f24... @-0x6e818 = 5.0f).
Identify 0x7b4's meaning during reconcile (VERIFY — probably the whiff-lunge/stop-related).

## Full write/read site map (nearest-symbol resolved)

player+0x774 (melee bar):
- WRITE: PreThink +0x4eb (regen, above); TSInit +0x357 (=100.0);
  UpdateMeleeBar @0x8650c (drain + zero + KFuPower send — the dossier's choke-point)
- READ: CTSGun::UpdateClientData +0x202 (client sync), CTSGun::CCombatBlock +0x8d (gate),
  VictimKatanaHold +0x44 / VictimKnifeHold +0x53 / VictimKungfuHold +0x16d (deflect gates)

player+0x7a8/0x7ac/0x7b0 (block windows):
- OPEN (=0.5): CTSGun::CCombatBlock (+0xbd/+0xe6/+0x105; register-stored float bits)
- DECAY: PreThink (above)
- CLEAR (=0): TSInit (+0x386..+0x39a all three); PreThink +0x599 (0x7a8, in the whiff-lunge
  clear block); CTSGun::CCombatAttack +0xd8d (0x7a8); **CTSStunt::CloseAttack +0x1d5 (0x7a8)**;
  **CTSStunt::GoWheel +0x498 (0x7a8)** ← the backkick path clears the kungfu block window
- READ: the three Victim*Hold deflect gates (addrs above)

Implication for the reconcile plan (S1/S3): both gates that were "live-trace before declaring
done" are now spec'd; S1 can land final (PreThink regen + TSInit init + UpdateMeleeBar drain),
and the m_flKungFuBlock→value-countdown conversion in S3 follows the PreThink decay law above.
