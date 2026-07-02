# gmsgTSTakeDamage — wire name "PTakeDam" (size 13)

Server global symbol: `gmsgTSTakeDamage` (nm: `0010348c D gmsgTSTakeDamage`; GOT slot 0x1675b4 = %ebx+0x17a4).
Registered in `LinkUserMessages__Fv` (0xc7db0) at 0xc83f8: `REG_USER_MSG("PTakeDam", 13)`
(wire string "PTakeDam" @ .rodata 0xfe57f). Client hook string present in client.dll .data @0x958d4.

IMPORTANT companion fact: the original ALSO registers stock `gmsgDamage = REG_USER_MSG("Damage", 12)`
(LinkUserMessages @0xc7e39-0xc7e4e, string @0xfe325) **but NEVER sends it** — the registration store is
the ONLY reference to the gmsgDamage GOT slot in the whole .so. Stock "Damage" is compat-only;
"PTakeDam" is the live damage-feedback message.

## WIRE FORMAT (byte-faithful swap contract)

Sent `MSG_PVS` (dest=4) at `pev->origin` of the VICTIM (`mov 0x0(%ebp),%eax; add $8,%eax` = &pev->origin), ed=NULL.

| # | Type | Bytes | Value |
|---|------|-------|-------|
| 1 | COORD | 2 | `ptr->vecEndPos.x` (TraceResult+0x14) — world hit point |
| 2 | COORD | 2 | `ptr->vecEndPos.y` (+0x18) |
| 3 | COORD | 2 | `ptr->vecEndPos.z` (+0x1c) |
| 4 | COORD | 2 | `vecDir.x` (the TraceAttack attack-direction arg) |
| 5 | COORD | 2 | `vecDir.y` |
| 6 | COORD | 2 | `vecDir.z` |
| 7 | BYTE | 1 | encoded damage: `(int)( min(flDamage*0.5, 127) + (bodyHit ? 128 : 0) )` |

Total 6x2+1 = **13** — matches the registered size. COORD = standard engine WriteCoord (value*8 as short).

BYTE encoding detail (disasm 0x80448-0x804d4, FP consts read from .rodata):
- `flDamage` at this point is the post-hitgroup-multiplier damage of THIS TraceAttack.
- Pre-scale: if `bitsDamageType & 0x2000000` (TS-specific DMG bit 1<<25 — VERIFY meaning) -> `flDamage *= 8.0` (float @0xf75fc) *for the encode only* (loaded fresh from the stack slot).
- Scale x**0.5** (float @0xf71e4), clamp to max **127.0** (float @0xf92c8).
- `bodyHit` flag (stack 0x6(%esp)) = `m_LastHitGroup in [2..5]` (HITGROUP_CHEST/STOMACH/LEFTARM/RIGHTARM; check at 0x8038f: `member@0x10c - 2 <= 3`) -> adds **128.0** (float @0xf7618), i.e. bit7 = torso/arm hit.
- FISTP with RC=truncate -> C-cast to int -> WRITE_BYTE.

Throttle (per victim): send only if `this->@0x750 <= gpGlobals->time`; on send set
`this->@0x750 = gpGlobals->time + 0.3` (double @0xf7fb8). Transient float member @0x750
(not in save tables / members_auto; suggest `m_flNextPTakeDamTime` — INVENTED name, mark provenance).
The throttle check happens BEFORE MessageBegin (jae 0x80830 skips the whole send).

## PURPOSE

Client-side damage-hit feedback: tells clients in the victim's PVS where a player was hit
(world hit point), from what direction, and a coarse damage magnitude with a "torso/vest zone"
flag in bit7. It REPLACES the stock `Damage` message's pain-direction/feedback role (the original
never sends stock Damage). Client effect: directional pain indicator + hit feedback (blood/pain);
the handler activates its HUD element.

## SERVER SEND SITES

- **Exactly one send site**: `CBasePlayer::TraceAttack` (`TraceAttack__11CBasePlayerP9entvars_sfG6VectorP11TraceResulti`
  @0x801cc, file `TS_player.cpp` per funcfile.tsv:483). Send sequence at **0x804d8-0x8055f**:
  MessageBegin(MSG_PVS, gmsgTSTakeDamage, pev->origin, NULL); 6x WriteCoord; WriteByte; MessageEnd.
- Registration: `LinkUserMessages` @0xc83f8.
- Send conditions inside TraceAttack (context, from disasm):
  - entry gate: skip whole function if `this->@0x71c > gpGlobals->time` (VERIFY: some invuln/protection timer);
  - `pev->takedamage != 0` (fcomps 0x16c(pev));
  - the 0.3 s per-victim throttle (`@0x750`) gates only the message, not the damage.
- Damage-calc context surrounding it (TraceAttack internals, for the re-project pass — VERIFY semantics):
  - `pev->iuser4 & 0x10c10` (entvars @0x250) -> `flDamage *= 0.7` (double @0xf7fc8) — kevlar/item bits;
  - `this->@0x524 > time && (bitsDamageType & DMG_BULLET)` -> `flDamage *= 0.7` — timed bullet-resist;
  - hitgroup switch stores `m_LastHitGroup` (this@0x10c, transient, members_auto count=3), HEAD path doubles
    damage (`fadd st0,st0` @0x8032c) and sets a separate headshot bool (0x7(%esp)) used AFTER the message
    for pain-sound selection (NOT in the wire byte); one hitgroup path multiplies by 0.5 (double @0xf6e00);
  - after MessageEnd: `bitsDamageType & 0x42` (DMG_BULLET|DMG_BLAST) -> blood/pain-sound section.

## CLIENT HANDLER BEHAVIOR (ground truth: gmsg_TSTakeDamage.c)

`FUN_1004bcd0` (forwarder 0x10049ac0, `this` = CTSHud-member object 0x100d43cc):
1. `BEGIN_READ(pbuf, iSize)` (FUN_100299b0)
2. 6x `READ_COORD` (FUN_10029b60): vec1=(hit point x,y,z) -> locals c/8/4; vec2=(dir x,y,z) -> locals 18/14/10
3. `READ_BYTE` (FUN_10029a10) -> encoded damage
4. `FUN_10048840(&vec1, &vec2, byte)` — the TS pain/damage-feedback routine (CalcDamageDirection-equivalent
   + hit-point effect; exact rendering + bit7 decode = VERIFY, client Ghidra project locked this pass)
5. `this->m_iFlags |= 1` (offset 0x10) — HUD element set active.

Read order confirms the field order above. No armor/health bytes — TSHealth/TSArmor carry those separately.

## OURS

**Status: absent (with a stock-substitute drift).**
- Server: `gmsgTSTakeDamage` / "PTakeDam" not registered, never sent.
  Drift: we SEND stock `gmsgDamage` ("Damage", 12) from `CBasePlayer::TakeDamage` (dlls/player.cpp:4444)
  — the original registers "Damage" but never sends it. Our send is inherited HL-SDK behavior, not TS.
- Client: no "PTakeDam" hook. Stock `CHudHealth::MsgFunc_Damage` (cl_dll/health.cpp:117) is doing the
  pain-direction job off our non-original Damage sends; health.cpp:211 comment explicitly notes
  DrawDamage/DrawPain are kept "until TS's own (PTakeDam/TSFade) is reconstructed".
- The audit's invented globals gmsgStatusText/gmsgStatusValue (stock messages, original has them) and
  gmsgWeapons ("Weapons", 8 — HL leftover, separate review) are NOT substitutes for this message; the
  real substitute is the stock Damage SEND.

## RECONCILE

Server (dlls/):
1. `dlls/UserMessages.h`: add `inline int gmsgTSTakeDamage = 0;`
   `dlls/UserMessages.cpp` `LinkUserMessages()`: `gmsgTSTakeDamage = REG_USER_MSG("PTakeDam", 13);`
   (original registers it there, not lazily). KEEP `gmsgDamage = REG_USER_MSG("Damage", 12)` (compat),
   but REMOVE the `MESSAGE_BEGIN(MSG_ONE, gmsgDamage, ...)` send in `CBasePlayer::TakeDamage`
   (dlls/player.cpp:4444) — original never sends it.
2. `dlls/player.h`: add transient `float m_flNextPTakeDamTime = 0;` (orig this@0x750, not saved;
   name invented — comment provenance).
3. `dlls/player.cpp` `CBasePlayer::TraceAttack` (:302) — after the hitgroup switch / m_LastHitGroup
   assignment (our function already has both, plus the RE'd body-impact sound that comes from the SAME
   original function), insert:

       if (m_flNextPTakeDamTime <= gpGlobals->time)
       {
           m_flNextPTakeDamTime = gpGlobals->time + 0.3f;
           float enc = flDamage * ((bitsDamageType & (1 << 25)) != 0 ? 8.0f : 1.0f) * 0.5f;
           if (enc > 127.0f) enc = 127.0f;
           if (m_LastHitGroup >= HITGROUP_CHEST && m_LastHitGroup <= HITGROUP_RIGHTARM)
               enc += 128.0f;
           MESSAGE_BEGIN(MSG_PVS, gmsgTSTakeDamage, pev->origin);
           WRITE_COORD(ptr->vecEndPos.x); WRITE_COORD(ptr->vecEndPos.y); WRITE_COORD(ptr->vecEndPos.z);
           WRITE_COORD(vecDir.x); WRITE_COORD(vecDir.y); WRITE_COORD(vecDir.z);
           WRITE_BYTE((int)enc);
           MESSAGE_END();
       }

   (Guarded by the existing `pev->takedamage` check. The x0.7 armor/powerup multipliers and the
   TS hitgroup multipliers are TraceAttack damage-calc parity items — separate reconcile, VERIFY
   against re.sh decompile of TraceAttack@0x801cc in the re-project pass.)
4. Naming: server global MUST be `gmsgTSTakeDamage` (audit matches by name); wire string MUST be
   "PTakeDam" (never "TSTakeDamage" on the wire — drop-in rule).

Client (cl_dll/):
5. Hook `HOOK_MESSAGE(PTakeDam)` in the damage-feedback element (natural absorber: `CHudHealth`
   in cl_dll/health.cpp, which already owns CalcDamageDirection/DrawPain — or a TS pain class if the
   client-side audit later shows a dedicated one). Handler mirrors the decompile: read 6 COORDs + BYTE,
   feed direction into the pain indicator (CalcDamageDirection style — VERIFY sign/use of hit
   point + bit7 once FUN_10048840 is decompiled), `m_iFlags |= HUD_ACTIVE`. Keep the stock "Damage"
   hook registered (the original client hooks both strings) but expect it never to fire from our server.
6. Retire health.cpp:211's dependency note once PTakeDam drives DrawPain.

VERIFY list for the re-project pass: DMG bit 0x2000000 meaning; @0x71c and @0x524 timer members;
iuser4 bits 0x10c10 (kevlar items); client FUN_10048840 internals (bit7 + hit-point use); whether
the throttle skip target (0x80830) also skips the blood section.
