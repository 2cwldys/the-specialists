# TS playtest verification checklist + per-feature test steps

What needs a human playtest (input-gated, can't be self-verified headlessly), with exact
steps. Also tracks the **silencer/`+alt1`** assembly findings (for when it's implemented)
and the **think-behavior** porting status. Update the Status column as items are confirmed.

Launch (this box; re-run `scripts/setup-xash-mod.sh` after any rebuild):
```
cd ~/Projects/goldsrc.net/xash3d-fwgs/dist/client-x64
LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana +sv_lan 1 +maxplayers 4
# cvar tests: append e.g.  +slowmatch 0.5   +roundtime 60   +damagemult 2
```

## A. Playtest checklist (needs your eyes/hands)

| # | Feature (commit) | How to test | Expected | Status |
|---|---|---|---|---|
| 1 | HUD weapon-bar (cont.53) | spawn; look top-left | weapon icon(s) in category columns (Beretta in col 2); active brighter | ⬜ |
| 2 | HUD ammo box (cont.52) | spawn; fire/reload | bottom-right: name / caliber / "Semi Auto" / "clip / reserve"; clip drops on fire, refills on reload | ⬜ |
| 3 | HUD health bar (cont.53) | take damage | bottom-right fill bar shrinks; turns red when low | ⬜ |
| 4 | Round clock (cont.51/54) | watch top-centre | M:SS counts down from `roundtime` (default 2:30); `+roundtime 60` → 1:00 | ⬜ |
| 5 | Predicted fire event (cont.56) | fire weapon_tsgun | viewmodel muzzleflash + brass shell ejects + fire sound + shoot anim | ⬜ |
| 6 | **Fire sound pitch** (cont.55) | fire, listen | NORMAL pitch (the old "low/slow" report — confirm fixed or still low; if low: which weapon? in slow-mo?) | ⬜ |
| 7 | Coldcock `+alt2` (cont.57) | `bind x +alt2`; press near an enemy/wall with a loaded gun | melee bash: knife hit/miss sound, ~25 dmg, 0.5s cadence | ⬜ |
| 8 | slowmatch (cont.59) | launch `+slowmatch 0.5` | whole match runs in slow-motion (movement/everything ~half speed) | ⬜ |
| 9 | No cmd/cvar spam (cont.54) | open console at startup | no "Unknown command" for hud_*/cl_*/mp_teammodels/roundtime/etc. | ⬜ |

Report per item: ✅ works / ❌ broken (+ detail) / ⚠️ partial.

## B. Silencer / `+alt1` — assembly findings (for implementation, then verify)

NOT yet implemented (the handler isn't cleanly mapped — tracked here so it isn't re-chased):
- `+alt2` = `CTSGun::AlternateAttack @0x81ba8` = **ColdCock** (done, cont.57) — NOT the silencer.
- `CTSGun::SecondaryAttack @0x717a8` = the **IN_ATTACK2 fire-type dispatcher** (no active item →
  `CCombatAttack` unarmed; fire-type 2 → `GrenadeFire`; type 3 → knife; …) — NOT the silencer.
- `CTSGun::Zoom(int) @0x67660` / `Unzoom` exist (forwarders) — the **scope/zoom** path for snipers.
- The **silencer toggle** (the `+alt1` attach/detach) has **no server-side string** to anchor on
  ("/HUD/Silencer" is client.dll only); the install state is the WeaponInfo **flags byte** (currently
  sent as 0) + the per-weapon supported-attachment mask (`player+0x798`, set from the weapon on deploy).
- The HUD attachment rows (`item_installed.spr` / `item_not_installed.spr`) in `FUN_1004d7e0` are gated
  by `player+0x798 & (1<<n)` (n: 0 Silencer, 1 Lasersight, 2 Flashlight, 3 Scope); installed = `+0x14` bit.
- **To implement:** decompile the `+alt1` input path (likely a server command / button bit) → find the
  attachment toggle that flips the install bit + (silencer) swaps the fire sound to `*-sil.wav`
  (those wavs exist: weapons/beretta/fire-sil.wav, aug/aug-fire-sil.wav, …) / (scope) calls Zoom.
  Then send the install mask in the WeaponInfo flags byte and draw the attachment rows + dots.
- **Verify once built:** `bind x +alt1`; with a silenceable pistol, press → fire sound changes to the
  silenced wav + the ammo box shows a lit "Silencer" row; with a sniper → zoom in/out.

## C. Think-behavior porting status (Phase 2)

Per-tick "think" reconstruction — PARTIAL:
- ✅ `CBasePlayer::PreThink` → `UpdatePowerUpState` (timed-powerup apply/expire, slow fields) — cont.14.
- ✅ `CBasePlayer::UpdateClientData` → stock + `TSUpdateClientData` (HUD messages) — cont.51/52.
- ✅ `CTSGameRules::Think` → CheckDeads/round timer/dynamic powerups/**slowmatch** (cont.37/45/26/59).
- ❌ `CBasePlayer::PostThink` TS calls NOT ported: **`CTSAnimation::Animate`** (the gait/animation
  subsystem, 14 funcs), **`CTSStunt::GoRoll`** (the stunt subsystem — dives/rolls/wall-flips, 25 funcs),
  and **`CheckPowerups`** (free fn). These are the big remaining TS-specific subsystems
  (reference/RECONSTRUCTION.md) — movement/animation, client-entangled.
