# cont94 — CBasePlayer::TraceAttack follow-ups: player+0x524, player+0x71c, full diff

RE of the two unmodeled members of the original `CBasePlayer::TraceAttack`
(nm `0x801cc` = Ghidra `0x901cc`, `TS_player.cpp`; preserved decompile
`reference/dossiers/cont91/decompiles/traceattack.c`), plus a line-by-line diff
of the original against our `dlls/player.cpp` `CBasePlayer::TraceAttack`
(~lines 303–392). Every claim below is instruction-verified in
`reference/ts_i386.so` (objdump; GOT base `0x165E10`; all `0x71c(%reg)` /
`0x524(%reg)` / `0x34(%reg)`-in-CTSStunt-range accesses exhaustively
enumerated).

---

## (a) player+0x524 — NOT a direct player member: it is `CTSStunt+0x34` = our `m_flBulletTimeEnd`

### Offset decomposition
`members_auto.tsv` lists `CBasePlayer 0x524 int`, but that row is only the
TraceAttack read seen through `this`=player. CTSStunt is embedded at
**player+0x4f0** (proven: `TSInit` @`0x82a76` does `lea 0x4f0(%ebp)` →
`Init__8CTSStuntP11CBasePlayer`; PreThink/PostThink/Duck/Jump do the same lea
before `Play`/`CheckForBreakables`/`GoRoll`/`CloseAttack`). So
**player+0x524 = CTSStunt+0x34**, which our `dlls/ts_stunt.h:103` already
declares: `float m_flBulletTimeEnd = 0; // +0x34 bullet-time end`.

### The exact TraceAttack condition (disasm `0x80220–0x8024f`)
```
flds 0x524(%ebp) ; fcomps gpGlobals->time ; test $0x41,%ah ; jne skip   ; skip when +0x524 <= time (STRICT >)
testl $0x2,0x74(%esp)                                       ; bitsDamageType & DMG_BULLET (0x2)
fldl  dc_0_7(@0xF7FC8 = 0.7 double) ; fmuls flDamage ; fstps flDamage
```
i.e. **`if (gpGlobals->time < m_TSStunt.m_flBulletTimeEnd && (bitsDamageType & DMG_BULLET)) flDamage *= 0.7f;`**
- It is a **time deadline** (absolute `gpGlobals->time` compare, strict `<`), not a flag.
- Only `DMG_BULLET` (0x2) qualifies — melee/blast/fall take full damage during the window.
- It is applied **after and independently of** the `pev->iuser4 & 0x10c10` ×0.7
  damping (disasm `0x80203–0x8021c`) — the two **stack** (0.49× if both hold).
- Both dampings run BEFORE the friendly-fire / `takedamage` early-outs (local
  float only, no side effect — order is behaviorally irrelevant).

### ALL writers of +0x524 in the binary (exhaustive)
Whole-binary scan for `0x524(%reg)` (any base register, any instruction form):
exactly **two** sites — the PLT `jmp *0x524(%ebx)` (not a member access) and the
TraceAttack read `0x80228`. Whole-CTSStunt-range (0x89fbc–0x8d400) scan for
this-relative `0x34(%reg)`: exactly **one** write —

| site | function | what it writes |
|---|---|---|
| `0x8a669` `fstps 0x34(%ebp)` | **`CTSStunt::GoBulletTime` @0x8a5b4** (ebp = `this`, arg0) | `this+0x34 = gpGlobals->time + 0.55f` (float const @vaddr `0xF7F80` = 0.55; same const also stored raw to `this+0x00` m_flStuntTime) |

- **`CTSStunt::Init` (@0x89fbc) does NOT clear +0x34** (it clears +0x00/04/08/0c/10/14/18/30/38, sets +0x1c=-1, +0x2c=0, +0x40=player — +0x34 untouched). No respawn reset — stale value is always in the past, harmless. Do NOT add a reset to our Init.
- No other function takes a CTSStunt pointer and stores to +0x34 (all six
  `lea 0x4f0(...)` sites immediately push+call a CTSStunt PLT method).

### Our side — the writer ALREADY exists and matches
`dlls/ts_stunt.cpp` `CTSStunt::GoBulletTime()` already does
`m_flBulletTimeEnd = gpGlobals->time + 0.55f;`. The trigger is the standing
matrix-dodge (`+alt1` on ground with horizontal speed < 80), **not the roll** —
`GoRoll` never touches +0x34; the "roll-dodge i-frame" framing in the task was
wrong. **The ONLY missing piece is the TraceAttack read.**

### Fix spec (a)
In `dlls/player.cpp` `CBasePlayer::TraceAttack`, immediately after the
`pev->iuser4 & 0x10c10` damping:
```cpp
// RE @0x80228: bullet-time matrix-dodge window (CTSStunt::GoBulletTime sets
// m_TSStunt.m_flBulletTimeEnd = time + 0.55) — a BULLET taken inside it is x0.7.
// Stacks with the iuser4 damping above.
if (gpGlobals->time < m_TSStunt.m_flBulletTimeEnd && (bitsDamageType & DMG_BULLET) != 0)
    flDamage *= 0.7f;
```
`CTSStunt` already has `friend class CBasePlayer` (ts_stunt.h:49); members are
private — access compiles as-is. Update the stale "not modeled here yet" comment
at player.cpp:309–312. Update `ts_stunt.h:103` comment to record the consumer
(`TraceAttack ×0.7 bullet damping, player+0x524`). No DEFER needed — the whole
loop (writer + consumer) closes with this one read.

---

## (b) player+0x71c — TRUE role: the SPAWN-PROTECTION deadline (our `m_flDisarmTime` — label too narrow, writers already correct)

### ALL accesses in the binary (exhaustive: every `0x71c(%reg)` form)
| addr | function | R/W | semantics |
|---|---|---|---|
| `0x82b26` | **`CBasePlayer::TSInit` @0x82a4c** | **WRITE (the ONLY one)** | `+0x71c = gpGlobals->time + 1.0` (double @`0xF6DF8`); same 1.0 also into `m_flNextAttack`(+0x12c)`= UTIL_WeaponTimeBase()+1.0`; plus shimmer (rendermode=1, renderamt=125, rendercolor 0,0,0, renderfx=14) |
| `0x82bb2` | `TSInit` | R | `UTIL_ScreenFade(this, color(200,200,250), 0.2f, (+0x71c − time)=1.0, 25, 0)` — fadeHold = remaining window |
| `0x801eb` | `TraceAttack` | R | **`if (time < +0x71c) return;`** — full no-damage early-out, and the early path does NOTHING else (no blood, no sound, no message; straight `jmp 0x808c1` = function epilogue). Strict `<`: at `time == deadline` damage flows. |
| `0x81460` | `PreThink` | R | `if (+0x71c < time && pev->rendermode != 0) { clear shimmer }` — the expiry clear |
| `0x8622c` | `CloseCombatAttack` | R | on the **VICTIM**: `if (bDisarm && victim+0x71c < time) victim->DropWeapon(-2, f)` — a spawn-protected victim can't be disarmed |
| `0x86806` | `ColdCockAttack` | R | same disarm gate (victim in %ebp) |
| `0x8c297` | `CTSStunt::CheckForBreakables` | R | same disarm gate on the swept victim (after `VictimKungfuHold`) |
| `0x902f0`,`0x9030c` | `CBasePlayerWeapon::DefaultDeploy` @0x90178 | R | `if (player+0x71c > time) m_flNextAttack = UTIL_WeaponTimeBase() + (+0x71c − time); else m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;` — deploying during protection re-arms next-attack to the protection remainder |

### Reconciliation with our `m_flDisarmTime`
**One member, one role, several consumers.** It is the spawn-protection
deadline; "disarm immunity" is just one of its four consumers (no-damage gate,
shimmer clear, disarm immunity, deploy next-attack push-out). We did NOT
mislabel a second member — there is no second member.

- **Writers: OURS ALREADY MATCH.** `dlls/ts_player.cpp:681` (`TSInit`) is the
  only write (`time + 1.0f`), exactly like the original; ts_player.cpp:922/1000
  melee-twin reads match the original's `victim < time` gates; player.cpp:1918
  PreThink shimmer-clear matches.
- **Original name: NOT recoverable** (transient member — not in the save tables
  / `members.tsv`; mp.dll exports don't name members). Recommend rename
  `m_flDisarmTime` → **`m_flSpawnProtectTime`** (invented name, VERIFY — keep the
  `// 0x71c` provenance and list all four consumers in the comment), and fix
  `dlls/player.h:434`'s "disarm-immunity gate for DropWeapon(-2) in the melee
  twins" to the full role. Purely cosmetic; audit tools don't match transient
  member names against the binary.

### Fix spec (b)
1. **TraceAttack gate** — very first statement of `CBasePlayer::TraceAttack`
   (before `takedamage`, before damping, before everything):
   ```cpp
   // RE @0x801eb: spawn protection (TSInit: m_flDisarmTime = time + 1.0) — a
   // protected player takes NO trace damage; the early path does nothing else.
   if (gpGlobals->time < m_flDisarmTime)
       return;
   ```
2. **DefaultDeploy** (`dlls/weapons.cpp:806`, server-only — cl_dll does not
   compile weapons.cpp, and the original client predicted the plain 0.5s path;
   `m_flNextAttack` reaches the client via clientdata anyway) — replace the
   stock `m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;` with:
   ```cpp
   // RE DefaultDeploy @0x902f0: deploying while spawn-protected pushes next-attack
   // out to the end of the protection window instead of the stock 0.5s.
   if (m_pPlayer->m_flDisarmTime > gpGlobals->time)
       m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + (m_pPlayer->m_flDisarmTime - gpGlobals->time);
   else
       m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
   ```
3. `CheckForBreakables`' disarm gate stays DEFERRED with its already-deferred
   player-victim path (dlls/ts_stunt.cpp:799 comment) — the consumer
   (close-combat run-through vs players) isn't reconstructed; adding the gate
   alone ships half a loop (defer-setter-without-consumer).

---

## Full diff: original TraceAttack vs dlls/player.cpp:303–392

Original flow (instruction-verified): **[1]** spawn-protect return → **[2]**
iuser4 ×0.7 → **[3]** bullet-time ×0.7 → **[4]** FF teammate return → **[5]**
`takedamage==0` return → **[6]** `m_LastHitGroup = ptr->iHitgroup` → **[7]**
hitgroup switch (head ×2 bullet / ×1.5 bit25, legs ×0.5, sets glast_headshot +
local `bHead`) → **[8]** compute `bArmoredHit` → **[9]** if dead: BulletDance +
corpse push → **[10]** throttled feedback block (msg + hit sound + pain sound)
→ **[11]** `AddMultiDamage` → **[12]** dmgreport alert.

| # | Divergence | Original (evidence) | Ours | Action |
|---|---|---|---|---|
| D1 | **Spawn-protect early return missing** | `0x801eb` (see (b)) | absent | ADD (spec b.1) |
| D2 | **Bullet-time ×0.7 missing** | `0x80228` (see (a)) | absent | ADD (spec a) |
| D3 | **Friendly-fire early return missing** | `0x8024f–0x802aa`: `CBaseEntity *pAtt = GetClassPtr((CBaseEntity*)pevAttacker); if (pAtt && pAtt->IsPlayer() /*vtbl+0xc8*/ && g_pGameRules->PlayerRelationship(pAtt, this) /*vtbl+0xcc, args (attacker, victim)*/ == GR_TEAMMATE /*==1*/ && friendlyfire.value == 0) return;` (cvar_t.value @+0xc; slots confirmed via vtables_original.json CHalfLifeMultiplay) | absent (stock FF handling elsewhere doesn't cover TraceAttack) | ADD after the dampings, before the takedamage check. `friendlyfire` extern in game.h; `#include "gamerules.h"` already present. NB: `GetClassPtr` on pevAttacker (original used GetClassPtr, not Instance) — mirror with `CBaseEntity::Instance(pevAttacker)`? NO — original literally calls `GetClassPtr<CBaseEntity>(a0)` (@`0x80256` PLT). Use `GetClassPtr((CBaseEntity*)VARS(...))`-equivalent: `GetClassPtr((CBaseEntity *)pevAttacker)`. |
| D4 | **`SpawnBlood`/`TraceBleed` are INVENTED** (stock leftovers) | no such calls anywhere in the original function | player.cpp:341–342 | Original relies on the client's `TSTakeDamage` handler for hit feedback; our cl_dll has **no TSTakeDamage hook yet** — removing these now would leave players with zero blood. **DEFER removal until the client TSTakeDamage handler is reconstructed** (defer rule: don't rip out the only current provider of the feedback loop). Mark with a TODO-invented comment. |
| D5 | **Dead-body branch missing** | `0x80300ish` (decompile l.84–100): `if (!IsAlive()) { m_TSAnim.BulletDance(this); if (!(pev->flags & FL_ONGROUND)) pev->velocity = pev->velocity + vecDir * 50.0f; }` — runs before the feedback block; corpse twitch + push. `CTSAnimation::BulletDance` exists in our ts_animation.cpp. | absent | ADD (place after m_LastHitGroup/hitgroup mults, matching original order) |
| D6 | **Hit sounds outside the 0.3s throttle + wrong DMG mask** | ALL feedback (TSTakeDamage msg, hit sound, pain sound) is inside `if (m_flNextPTakeDamTime < time)`; hit-sound gate is `bitsDamageType & 0x42` = **DMG_BULLET\|DMG_BLAST** (`0x80568`: `testl $0x42`) | sounds unconditional, gated `DMG_BULLET\|DMG_CLUB` (0x82) | RESTRUCTURE: move sounds inside the throttle block; mask → `(DMG_BULLET | DMG_BLAST)` |
| D7 | **Headshot sound: invented `headshot1.wav`** | `RANDOM_LONG(0,1)`: 0→`player/headshot.wav`, 1→`player/headshot2.wav` (strings @0xF93C5/0xF93D9; only these two exist) — played when local `bHead` (head hitgroup + bullet-type) | `RANDOM_LONG(0,2)` with sprintf `headshot%d` → nonexistent `headshot1.wav` 1/3 of the time | FIX |
| D8 | **Kevlar-hit / +128 flag condition wrong** | one local `bArmoredHit = (pev->armorvalue != 0) && (bitsDamageType & DMG_BULLET) && IsAlive() && (hitgroup >= HITGROUP_CHEST(2) && hitgroup <= HITGROUP_RIGHTARM(5))` (decompile l.80–83) drives BOTH the kevlarhit.wav choice AND the `+128` on the TSTakeDamage byte | kevlar sound: `m_bHasKevlar && (GENERIC\|CHEST\|STOMACH)`; +128: hitgroup 2..5 only | FIX: compute `bArmoredHit` once (use `pev->armorvalue`, include arms, exclude GENERIC, require DMG_BULLET + IsAlive) and use it for both. |
| D9 | **Body-hit sound params** | `RANDOM_LONG(0,3)` → bodyhit1–4 (ours 1..4 = same distribution, keep); `EMIT_SOUND_DYN(edict, CHAN_BODY, snd, 1.0, /*attn*/1.25, 0, /*pitch*/95+RANDOM_LONG(0,0x1f), /*slowmo*/0.5f)` — same vol/attn/pitch for headshot + kevlar sounds | `VOL_NORM, ATTN_NORM(0.8), PITCH_NORM(100)` | FIX vol/attn/pitch. The trailing 0.5f is the TS 8-arg EMIT_SOUND_DYN (D14). |
| D10 | **Pain sounds missing** | inside the throttle, after the hit sound, regardless of the 0x42 mask: `if (IsAlive()) switch (RANDOM_LONG(1,7)) { 1: pain2.wav; 2: pain3.wav; 3: pain4.wav; default: silent; }` — `EMIT_SOUND_DYN(edict, CHAN_VOICE, snd, 1.0, 1.25, 0, 100, /*slowmo*/0.15f)` (consts @0xF6A14/0xF92CC/0xF6F10). ~3/7 chance. | absent | ADD |
| D11 | **dmgreport alert missing** | after AddMultiDamage: `if (g_flWeaponCheat != 0 && CVAR_GET_FLOAT("dmgreport") != 0) ALERT(at_console, "%s damaged %s (%f hp)\n", STRING(pevAttacker->netname), STRING(pev->netname), flDamage);` (`0x80842–`; g_flWeaponCheat = the sv_cheats snapshot — our tree uses `CVAR_GET_FLOAT("sv_cheats")`, cf. TS_combat.cpp:84) | absent (cvar `dmgreport` exists, ts_cvars.cpp:48) | ADD. Note the != 0 test here vs TS_combat's `> 1.0f` tiers — both faithful to their own sites. |
| D12 | AddMultiDamage order | called AFTER the feedback block | ours line 343, before | Reorder for source parity (behaviorally neutral — damage applies at ApplyMultiDamage). |
| D13 | Throttle compare | `m_flNextPTakeDamTime < time` (strict) | `<=` | Align (cosmetic). |
| D14 | **EMIT_SOUND_DYN is a TS 8-arg overload** | `EMIT_SOUND_DYN__FP7edict_siPCcffiif` @0xd0a8c: extra trailing float `f` = slow-motion pitch floor — effective pitch = `pitch * (f + (1−f) * pev->fuser1)` (fuser1 = the entity's timescale; sounds deepen in slow-mo; f=0.5 body hits, 0.15 pain voice). Our util.h EMIT_SOUND_DYN is the stock 7-arg. | stock | **Separate work item** (repo-wide: every original call site passes the 8th arg). For THIS function, pass-through pitch as spec'd; add the overload when the slow-mo audio pass happens. Flag in the ledger. |

Verified-matching (no action): m_LastHitGroup assignment; hitgroup switch (head
×2 bullet / ×1.5 bit25 / glast_headshot, legs ×0.5, no torso mult); TSTakeDamage
wire (MSG_PVS @ pev->origin, 3 COORD endpos + 3 COORD dir + 1 byte
`min(dmg*(bit25?8:1)*0.5, 127) [+128 if bArmoredHit]`, truncating int cast);
throttle re-arm `time + 0.3`; iuser4 0x10c10 ×0.7.

### Suggested reconstructed shape (server, inside `takedamage` path)
```cpp
if (gpGlobals->time < m_flDisarmTime)      // D1 (rename: m_flSpawnProtectTime)
    return;
bool bHead = false;
if (pev->iuser4 & 0x10c10) flDamage *= 0.7f;
if (gpGlobals->time < m_TSStunt.m_flBulletTimeEnd && (bitsDamageType & DMG_BULLET) != 0)
    flDamage *= 0.7f;                      // D2
CBaseEntity *pAtt = GetClassPtr((CBaseEntity *)pevAttacker);   // D3
if (pAtt && pAtt->IsPlayer() && g_pGameRules->PlayerRelationship(pAtt, this) == GR_TEAMMATE
    && friendlyfire.value == 0)
    return;
if (pev->takedamage == 0) return;
m_LastHitGroup = ptr->iHitgroup;
switch (...) { /* unchanged head/leg mults, sets bHead + glast_headshot */ }
bool bArmoredHit = pev->armorvalue != 0 && (bitsDamageType & DMG_BULLET) != 0 && IsAlive()
    && m_LastHitGroup >= HITGROUP_CHEST && m_LastHitGroup <= HITGROUP_RIGHTARM;   // D8
if (!IsAlive()) {                          // D5
    m_TSAnim.BulletDance(this);
    if (!(pev->flags & FL_ONGROUND)) pev->velocity = pev->velocity + vecDir * 50.0f;
}
if (m_flNextPTakeDamTime < gpGlobals->time) {   // D6/D13 — msg + ALL sounds inside
    m_flNextPTakeDamTime = gpGlobals->time + 0.3f;
    /* TSTakeDamage msg (byte uses bArmoredHit for +128) */
    if (bitsDamageType & (DMG_BULLET | DMG_BLAST)) {
        /* bHead ? headshot{,2}.wav (RANDOM_LONG(0,1))            D7
           : bArmoredHit ? kevlarhit.wav                          D8
           : bodyhit1-4  (RANDOM_LONG(1,4))
           all: CHAN_BODY, 1.0, 1.25, 0, 95+RANDOM_LONG(0,0x1f)   D9 */
    }
    if (IsAlive()) { /* RANDOM_LONG(1,7): 1..3 -> pain2/3/4, CHAN_VOICE,1.0,1.25,0,100  D10 */ }
}
SpawnBlood(...); TraceBleed(...);          // D4 — INVENTED vs original; keep w/ TODO until client TSTakeDamage exists
AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);   // D12
if (CVAR_GET_FLOAT("sv_cheats") != 0 && CVAR_GET_FLOAT("dmgreport") != 0)   // D11
    ALERT(at_console, "%s damaged %s (%f hp)\n", STRING(pevAttacker->netname), STRING(pev->netname), flDamage);
```

## Verification protocol (user playtest)
1. Spawn → get shot within 1 s: zero damage, no hit sound/msg; at >1 s damage flows; shimmer clears ~1 s.
2. Deploy a weapon immediately on spawn: can't fire until the 1 s mark (not 0.5 s).
3. Standing matrix-dodge (+alt1, standing): bullets during the 0.55 s window hit for 70% (chest pistol shot).
4. mp_friendlyfire 0, teamplay: teammate bullets do nothing at all (no blood/sound).
5. Headshot sounds: only headshot.wav/headshot2.wav (watch console for a missing headshot1.wav precache error = regression gone).
6. Kevlar arm-shot plays kevlarhit; dead body shot off a ledge twitches + drifts.
7. sv_cheats 1 + dmgreport 1 → console "X damaged Y (n hp)".
