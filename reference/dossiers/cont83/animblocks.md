# cont.83 — Write_* anim-block + reload/stackable column extraction

Disasm-extracted from `recovered/scratch-cont81/ts.dis` (the cont82 README one-liner pattern:
`mov[wl] $V,0xOFF(%eax)` per `Write_<w>__FP13s_weapon_info` body). Applied to
`dlls/ts_weapon_info.{h,cpp}` in the gun frame-machine unit.

## wi+0x14 — flReloadTime (float, seconds)

Whole-clip reload delay (ReloadWeapon's flDuration via Reload); the PUMP shotguns' 0.3 is the
per-shell insert period (ShotgunReload phase 1). By tsweapon_t index:

glock18 1.5 · beretta 1.2 · uzi 2.2 · benellim3 0.3 · m4 2.8 · mp5sd 2.3 · mp5k 2.3 ·
beretta_ak 2.4 · mk23 1.5 · mk23_ak 2.5 · usas12 3.0 · desert 2.1 · ak47 2.5 · fnh 1.5 ·
aug 2.4 · uzi_ak 3.0 · skorpion 1.0 · barrett 2.0 · pdw 2.5 · spas12 0.3 · gold 2.3 ·
glock22 2.0 · mac10 2.5 · m61 1.0 · knife 1.0 · mossberg 0.3 · m16 2.8 · ruger 1.3 ·
flag 1.0 · fnh_ak 3.0 · bull 2.0 · m60 3.0 · sawedoff 1.7 · katana 1.0 · sealknife 1.0 ·
contender 1.35 · skorpion_ak 2.0 · (melee unset = 0)

## wi+0xdc — iStackable (byte)

= 1 for knife, sealknife, m61 (flag/katana explicitly 0). Clip = a COUNT of units; PickUpWeapon
charges slotCost × the world item's clip (@0x8314c); MultiplyClips adds units funded by free
slots (@0x71308).

## Anim blocks (29 shorts each: normal wi+0x252, hold wi+0x28c)

Format: weapon → [0] idle / h[0] hold-idle / [8] shell-retained reload (h[8]) / [28] COLDCOCK vm
anim (h[28]). 65535 = -1 (no hold block / unset). [0] = 0 for every weapon (idle seq 0).

| weapon | h[0] | [8] | h[8] | [28] | h[28] |
|---|---|---|---|---|---|
| ak47 | 4 | | | | |
| aug | - | | | 4 | |
| barrett | - | | | 5 | |
| benellim3 | - | 2 | | 10 | |
| beretta_akimbo | 13 | | | | |
| desert | 6 | | | 11 | 11 |
| m16 | 7 | | | 15 | 15 |
| m4 | 7 | | | 15 | 15 |
| m60 | - | | | 5 | |
| mac10 | - | | | 6 | |
| melee | - | 6 | | | |
| mk23 | 5 | | | | |
| mk23akimbo | 6 | | | | |
| mossberg | 3 | 8 | 13 | | |
| mp5k | 7 | | | | |
| mp5sd | - | | | 6 | |
| sawedoff | 5 | 9 | 10 | 13 | 13 |
| spas12 | - | 2 | | | |
| usas12 | - | | | 4 | |
| uzi | 7 | | | | |

The **[28] coldcock column** (aug 4 · barrett 5 · benelli 10 · desert 11 · m16/m4 15 · m60 5 ·
mac10 6 · mp5sd 6 · sawedoff 13 · usas12 4 — exactly the iColdCockDamage set) is the blocker the
S5/S6-deferred coldcock.sc/closecombat.sc event swaps were waiting on (dossier-ev-melee.md) —
the gun-event tail task is now UNBLOCKED on data.

## reload.sc wire contract (ShotgunReload @0x74aac / Reload @0x74d5c)

`PlaybackEvent(FEV_NOTHOST, player, gun+0x970, 0, zero, zero, fparam1, 0, iparam1=weaponType,
iparam2=item->attachEquipped, 0, 0)` with fparam1 the phase discriminator:
**0 whole-clip · 8.0 pump-open · 9.0 shell-insert · 10.0 pump-close**.
empty.sc (gun+0x968): all params zero. coldcock.sc (gun+0x966): fparam1=vecDir?, iparam1=type,
iparam2=attach, gated on item->bOwned — decompile ColdCock @0x71dd0 for the exact packing when
landing the event swap.
