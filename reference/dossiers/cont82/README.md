# cont.82 RE working set (typed decompiles produced this session)

Preserved from the cont.82 session scratchpad. All from `reference/ts_i386.so` (dc.sh server,
Ghidra addr = nm + 0x10000) unless noted; regenerate any missing piece the same way.

- `secondaryattack.c` — CTSGun::SecondaryAttack @0x717a8 (the dispatcher; applied in S7a).
- `bouncetouch.c` — CTSKnife::BounceTouch @0x6decc (applied in S8 — embed/glance/tail truth).
- `grenade.c` — GrenadeFire @0x732f0 + GrenadeIdle @0x74f18 + TSExplosion @0x6d01c
  (the cook machine + explosion — all applied).
- `checkforwires.c` — CBasePlayer::CheckForWires @0x82f08 (applied with the Wires unit).
- `rulesinit.c` — CTSGameRules::Init @0x67d6c (rules+0 seeding; the SrvSett cache inits).
- `installgamerules.c` — the InstallGameRules region @0xc60cc (DecideGamePlay caller).
- `screenfadewrite.c` — UTIL_ScreenFadeWrite @0xdcc54 (TSFade; applied).
- `bulletpaths.c` — CBulletManager::UpdateBullet @0x5ef5c (the falloff V1 gate; applied in F3).
- `powerupspawn.c` — CTSPowerUp::Spawn @0x88258 (the zero-hull + renderfx truth; applied).
- `addentity.c` — client.dll HUD_AddEntity @0x447f0 (iuser4 distance-fade; NO glow — see the
  dropped-gun glow saga in the ledger).
- The five client melee event handlers + `dossier-ev-melee.md` live in `../cont81/`
  (ev_knife/ev_katana/ev_draw/ev_coldcock/ev_closecombat/ev_helpers.c) — the complete
  presentation contract incl. the melee pitch formula and the wi+0x252 anim-block model.

Key disasm-extraction one-liners used this session (the Write_<weapon> column pattern —
reuse for any missing s_weapon_info column): grep each `<Write_x__FP13s_weapon_info>:` body in
`recovered/scratch-cont81/ts.dis` for `movl $V,0xOFF(%eax)` / `movw $V,0xOFF(%eax)`.
⚠ TWO TRAPS: (1) SPARTA — +0x1c/+0x28/+0x30/+0x34 (damage/spread/viewkick/recoil) in Write_
bodies are the DEAD stage; Sparta() overwrites them for all firearms — never extract those four
from Write_ ([[sparta-overrides-write-weapon]]). (2) movl PAIRS — a movl at a short-array offset
writes TWO shorts (low+high); the single-value regex silently drops the odd entries (this hid
half the anim blocks until cont.83).
Columns already extracted this way: iPrice(+0xd8), iKnifeType(+0x20, incl. m61=4),
iSwingAnims(+0x270..274), iBody*(+0x128..130), the m61/knife/sealknife/katana anim blocks.
