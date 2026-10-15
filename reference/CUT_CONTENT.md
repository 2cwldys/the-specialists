# Content cut from TS 3.0 that still leaves traces

What older TS versions had and 3.0 dropped, as far as the shipped 3.0 files show. Checked 2026-10-15
against `reference/ts_i386.so`, `client.dll`, `mp.dll`, `reference/coverage/funcfile.tsv` (the original
source file of each function) and the content in `recovered/ts3-linux/ts/`.

| Item | What is left | Restorable |
|---|---|---|
| Armor powerup (bit 0x80) | All the code except the gate | Yes, server only |
| Steyr TMP | All its content; its weapon slot went to the Skorpion | Yes, with a protocol change |
| SPAS 15 | A source file name; its slot went to the Contender G2 | No, nothing to restore |
| Bullet dodge stunt | Changelog: "Removed bullet dodge stunt until something better can be thought up" | Not checked |

## Armor powerup (0x80)

The powerup bits are 1 slow motion, 2 infinite clip, 4 kung fu, 8 slow pause, 16 double firerate,
32 grenade, 64 health, 128 armor, 256 low gravity (superjump). The FGD lists every one but 128.

- Still there: `PlayerAddPowerUp` gives kevlar for it through `CBasePlayer::TakeArmor` (@0x87c16), the
  powerup model has its icon (`PUBODY_ARMOR`), and it is in the instant-powerup mask, so it needs no HUD
  timer; the kevlar shows on the normal HUD.
- The gate: `CTSGameRules::GetRandomPowerup` (@0x87894) always skips bit 0x80, and a `ts_powerup` whose
  explicit type has it set is re-rolled (`(char)type < 0`, @0x88047).
- Restoring it: lift those two checks, behind a cvar, defaulting off (a reTS deviation). Server only; a
  TS 3.0 client sees a kevlar pickup.

## Steyr TMP (weapon slot 17)

The Skorpion took the TMP's slot. Its weapon data function, `Write_skorpion`, is in the original's
`TS_tmp.cpp`, and it still fires `weapons/tmp/fire.wav` and `weapons/tmp/fire-sil.wav`.

- Still shipped, used by nothing: `models/v_tmp.mdl`, `p_tmp.mdl`, `p_tmp_sil.mdl`, `w_tmp.mdl`,
  `sprites/weapons/tmp.spr`, the `sound/weapons/tmp/` folder.
- No TMP code is left: no data function, no name, no model reference in any of the three binaries.
- Restoring it: a new weapon id (38) with its own data, buy menu entry, HUD and prediction, in both the
  server and the client. Weapon ids go over the wire, so a TMP would only work between a reTS server and
  reTS clients; a TS 3.0 client would not know it. Its stats would have to be invented or taken from an
  older TS version's binary.

## SPAS 15

Changelog 2.x: "Cut SPAS 15. Not enough time to make it look good." The original's `TS_spas15.cpp` holds
only `Write_contender`: the Contender G2 took the slot. No SPAS 15 model, sound or string ships.

## Found along the way: a TMP model on the silenced Skorpion

reTS's silenced third-person model table (`dlls/ts_weapon_info.cpp`, `kSil`) maps slot 17 to
`models/p_tmp_sil.mdl`, and its comment says every entry is a literal in the original's `Write_`
functions. That one is not: the original has sixteen `p_*_sil.mdl` strings and no `p_tmp_sil`, and
`Write_skorpion` (0x8d3f0) references only the Skorpion's v/p/w models, its sprite, `22_shell.mdl` and the
two TMP sounds. A silenced Skorpion in reTS shows the TMP's silenced model to other players; TS 3.0 does
not. Not yet fixed: the fix is dropping the row, after checking what Deploy shows in the original when
the silenced model is unset.
