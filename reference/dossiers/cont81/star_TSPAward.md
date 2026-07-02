# ★ TSPAward — send site, source member, PRODUCERS (SETTLED; member is a status/award byte)

Ground truth: UpdateClientData decompile (`star_decomp.txt`), GOT scan (disp 0x18cc → exactly 2
refs: registration + 0x8245d in UpdateClientData → **sole send site confirmed**), objdump scan of
all writes to `player+0x74c`, decompiles of SetPlayerRespawnTime / SetPlayerTheOne / Killed
(`star_decomp2.txt`) and disasm context of PlayerKilled / PreThink / RestartRound.

## Offset decode

HUD_MESSAGES' `player[0x1d3]` is dword-index notation: **0x1d3×4 = 0x74c**. So:
**source = byte @0x74c**, **last-sent cache = byte @0x74d** (dossier's "0x1d3" and our search for
"+0x1d3 writers" resolve to 0x74c). (Adjacent 0x74e is a different byte — set to 1 in
SetPlayerTheOne, unrelated.)

## The send (UpdateClientData, inside the 0.1 s throttle)

```c
if (this->field_0x74d != this->field_0x74c) {
    this->field_0x74d = this->field_0x74c;
    MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerAward);            // dest=2, MSG_ALL CONFIRMED
    WRITE_BYTE( pfnIndexOfEdict(edict) );
    WRITE_BYTE( (byte)this->field_0x74c );
    MESSAGE_END();
}
```
Wire = `"TSPAward"`, 2: BYTE entindex, BYTE value — exactly as the dossier. On-change only; no
ResetHUD-forced resend anywhere (cache 0x74d written only here; fresh player objects zero both).

## Producers — every writer of +0x74c (complete objdump inventory)

| value | writer | context |
|---|---|---|
| 0 | `CBasePlayer::TSInit` (+0x1e9, 0x82c35) | spawn-init clear |
| 1 | `SetPlayerRespawnTime(player,int,bool)` @0x65ff4 (TS_gamerules.cpp) | set when computed respawn time (+0x770) < 0 → player is OUT (no respawn) |
| 0xb (11) | same fn | respawn time ≥ 0 → player will respawn |
| 1 / 0xb | `CBasePlayer::Killed` @0x7ed5c (+0x2a2/+0x2ac) | repeats the same +0x770<0 test right after calling SetPlayerRespawnTime(clamp(respawntime cvar,5,30)) |
| 0xb | `CTSGameRules::RestartRound` @0x67920 (+0x200) | per-player round reset (also zeroes powerups, +0x7ba deaths, ...) |
| 1 | `CBasePlayer::PreThink` (+0x727, 0x80ff3) | last-man-standing respawn window (+0x7bc) expired: +0x770=-1, red TSHudMessageOne(0xc3,0x2d,0x2d,0x10,0x14,"You have to wait until the next round."), CallCheckDeads() |
| 0xa (10) | `CTSGameRules::SetPlayerTheOne` @0x7b078 | The One anointment (150 hp, speed 330, PwUp 4 + 0x100 msgs, held powerup 8) |
| 4 | `CHalfLifeMultiplay::PlayerKilled` (+0x1339, killer) | special-kill tier: also grants killer held powerup 8 (bullet-time), time 2, if he holds none |
| 3 | PlayerKilled (+0x138d, killer) | tier: killer frags += 60.0 (capped 150.0), local flag 0x3d branch |
| 2 | PlayerKilled (+0x13ed, killer) | tier: killer frags += 30.0 (capped 150.0), local flag 0x3e branch |

All three PlayerKilled writes are guarded `if (current != 0xa) write;` — **an active The One's
award byte (10) is never overwritten by kill awards**.

## Meaning of the member

NOT purely a "point-award popup" id — it is the player's broadcast **award/status byte**:
- 1 = out of round (no respawn left), 11 = respawning, 10 = The One, 2/3/4 = kill-award tiers
  (tier conditions = locals set earlier in PlayerKilled — killing-spree/special-kill thresholds;
  exact predicate + client Draw presentation per id remain the known client-side VERIFY),
  0 = cleared.
Because it's MSG_ALL keyed by entindex, every client's stride-8 per-player table (DAT_101901f4)
tracks all players' status — usable by scoreboard/observer UI, not just popups.

## Reconcile deltas vs dossier plan
- Land the member as the byte our audit will name from members_re curation (provenance +0x74c,
  not 0x1d3); cache +0x74d.
- The producer half is NO LONGER VERIFY-gated: wire the six producer sites above
  (TSInit, SetPlayerRespawnTime, Killed, RestartRound, SetPlayerTheOne, PreThink) — these are
  all in functions we already reconstruct. Only the PlayerKilled tier PREDICATES (which kill
  earns 2 vs 3 vs 4, +frag bonuses 30/60 cap 150) need the PlayerKilled decompile pass before
  wiring those three (they sit ~+0x1300 into the big fn; flags at locals 0x3d/0x3e).
