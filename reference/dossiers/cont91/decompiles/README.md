# cont.93 preserved decompiles + regeneration recipe

Ghidra decompiles captured while fixing the cont.93 damage/kill-feed/drop bugs. The typed
`re`/`client`/`mp` Ghidra projects are on-disk, so anything here is regenerable with
`scripts/ghidra/dc.sh server 0x<nm-addr>` (Ghidra addr = nm + 0x10000) or `re.sh <Class>`.

## Preserved (in this dir)
- `traceattack.c` — `CBasePlayer::TraceAttack` @nm 0x801cc. **The custom TS damage model** (hitgroup
  mults head ×2 bullets-only/legs ×0.5/torso ×1, ×0.7 slow-mo damping, kevlar, hit/pain sounds,
  gmsgTSTakeDamage). Ours had inherited the STOCK-HL version → the "2-shot beretta"/insta-kill.
- `closeattack.c` — `CTSStunt::CloseAttack` @nm 0x8c630. The jump auto-kick (gates + clamp(speed×K,10,20)×2).
- `tsthink.c` — `CTSGameRules::Think` @nm 0x67204. First block = the `globalslow`/SetGlobalSlowMotion driver.

## Regenerate these (were in scratchpad, cleared) — exact addresses:
- `dc.sh server 0x7ed5c` — `CBasePlayer::Killed` (death weapon-drop loop: DropAmmo/DropFlag then
  DropWeapon(type,0) over all types + a 2nd drop when get_weapon_info(type).iAkimboType != -1).
- `dc.sh server 0x84900` — `CBasePlayer::DropWeapon` (akimbo → TWO base singles; single/akimbo pair logic).
- `dc.sh server 0x795e4` — `CHalfLifeMultiplay::PlayerKilled` (kill-feed weapon name = get_weapon_info(type)+0x44).
- `dc.sh server 0x78f7c` — `CHalfLifeMultiplay::DeathNotice` (weapon name is a PARAMETER; caller builds it).
- `dc.sh server 0xa2430` — `CBaseMonster::TakeDamage` (NO DamageForce/velocity push — knockback was invented).
- `dc.sh server 0xa2d08` — `DamageForce` (×5, cap 1000 — faithful).
- `dc.sh server 0x85f88` — `CBasePlayer::CloseCombatAttack` (melee knockback = |attacker vel|·dir, z=235, >30).
- `dc.sh server 0xa0294` — `UpdateClientData` free fn (the vuser weapon-state pack — cont.91 shotgun).
- `dc.sh client 0x450c0` — client.dll `FUN_100450c0` (bullet whizz; radius _DAT_1007cccc=75.0 @rva 0x7cccc).
- `dc.sh server 0xb5580` — `cmd_globalslow` (percent → 0..2 scale). `dc.sh server 0x67204` = Think driver.

## rcon.py — drive the live rig (I set rcon_password lol on the test server)
`python3 reference/dossiers/cont91/decompiles/rcon.py status` / `... "changelevel ts_hammertime"` /
`... "globalslow 50"`. Uses the GoldSrc challenge/response over UDP to 127.0.0.1:27015.
