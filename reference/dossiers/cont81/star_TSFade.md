# ★ TSFade — ScreenFade family decompiled; GiveSlowMul division placement (SETTLED)

Ground truth: typed decompiles (`star_decomp.txt`) of `UTIL_ScreenFadeWrite` @ nm **0xdcc54**,
`UTIL_ScreenFadeBuild` @ **0xdcb94**, `UTIL_ScreenFade` @ **0xdcdec**, `UTIL_ScreenFadeAll`
@ **0xdcd7c** (all util.cpp), plus GOT scans for gmsgFade/callers.

## UTIL_ScreenFadeWrite(const ScreenFade&, CBaseEntity*) — the only sender

```c
if (!pEntity) return;
if (!pEntity->IsNetClient()) return;                      // gate confirmed
MESSAGE_BEGIN(MSG_ONE, gmsgTSFade, NULL, pEntity->edict);
WRITE_SHORT( (int)round( (long double)fade.duration / pEntity->GiveSlowMul() ) );  // vtbl call #1
WRITE_SHORT( (int)round( (long double)fade.holdTime / pEntity->GiveSlowMul() ) );  // vtbl call #2
WRITE_SHORT( fade.fadeFlags );
WRITE_BYTE(r); WRITE_BYTE(g); WRITE_BYTE(b); WRITE_BYTE(a);
MESSAGE_END();
```

**Division placement (the ★ question): the division happens on the ALREADY-FIXED 4.12 ushort
fields, inside Write, per recipient** — Build stores plain `FixedUnsigned16(t, 4096)` shorts and
Write divides the ushort by `GiveSlowMul()` (long-double FPU divide, round-to-nearest to int).
`GiveSlowMul()` is called **twice** (once per time field). Struct layout in the fixed shorts is
untouched — dividing fixed-point by a scalar ≡ dividing the float time, so precision-wise either
works, but source parity = divide the shorts in Write.

## UTIL_ScreenFadeBuild(ScreenFade&, color, fadeTime, holdTime, alpha, flags)

```c
fade.duration = FixedUnsigned16(fadeTime, 4096);   // 4.12 confirmed
fade.holdTime = FixedUnsigned16(holdTime, 4096);
fade.r/g/b = round(color.x/y/z); fade.a = alpha; fade.fadeFlags = (short)flags;
```
Build/Write split exists in the original exactly as in stock SDK — no rework needed beyond the
Write body.

## UTIL_ScreenFade / UTIL_ScreenFadeAll

- `UTIL_ScreenFade(ent, ...)` = Build once + Write(ent).
- `UTIL_ScreenFadeAll(...)` = **Build ONCE**, then `for i in 1..maxClients:
  Write(fade, UTIL_PlayerByIndex(i))` — **per-recipient scaling confirmed** (each player's own
  GiveSlowMul divides inside Write; NULL players tolerated by Write's null gate).

## Stock ScreenFade is registered but NEVER sent

`gmsgFade` GOT slot (disp 0x1dac) has exactly **one** code reference — the registration store in
LinkUserMessages (0xc80b8). **No original path emits stock "ScreenFade"** (dossier assumption
confirmed). Our current UTIL_ScreenFadeWrite → gmsgFade send must be rerouted to gmsgTSFade;
keep the `REG_USER_MSG("ScreenFade", 10)` registration (#29 in the order).

## Complete caller list (objdump, all 7 call sites of ScreenFade/ScreenFadeAll)

| caller | site | fade |
|---|---|---|
| `TSExplosion(...)` (TS_effects) | +0x240 (0x6d25c) | flashbang/explosion flash |
| `CBasePlayer::TakeDamage` | +0x5a0 (0x80034) | damage feedback fade |
| `CBasePlayer::TSInit` | +0x17c (0x82bc8) | spawn-time fade (clear/spawn flash) |
| `CFade::Use` (env_fade) | ×2 UTIL_ScreenFade (0xad8b7, 0xad939) + ×1 UTIL_ScreenFadeAll (0xad989) | stock env_fade paths |
| `CRevertSaved::Use` | UTIL_ScreenFadeAll (0xcde81) | stock |

(Write's only direct callers are ScreenFade/ScreenFadeAll themselves — no bypass exists.)
All callers absorb the reroute with zero call-site changes, as the dossier planned. Note our
extra callers (gauss.cpp etc.) are stock-SDK leftovers to reconcile separately; the original's
player-damage fade lives in TakeDamage (ties into the PTakeDam/TraceAttack reconcile).

## Client half — unchanged from dossier
Handler stores the 10-byte record @0x101914a8 + latches receive time @0x10190b5c; application
mechanism still a client-project VERIFY (reader of those globals). Wire format = SHORT dur,
SHORT hold, SHORT flags, BYTE r,g,b,a — confirmed against both ends.
