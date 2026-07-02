# gmsgTSKickAlign — analysis (DEAD global; no wire message exists)

## WIRE FORMAT

**NONE — this message has no wire format because it was never registered.**

Evidence (all lock-free binary queries against `reference/`):

- `nm reference/ts_i386.so` → `001034ac D gmsgTSKickAlign` — a 4-byte exported
  `.data` global (an `int`, zero-initialized like every other `gmsg*`).
- `rz-bin -zz reference/ts_i386.so | grep -i kickalign` → the ONLY hits are the
  symbol name itself in `.dynstr`/`.strtab`. There is **no `"TSKickAlign"`
  message-name string anywhere in the binary** — `REG_USER_MSG` requires the
  name string, so it is impossible for this message to be registered. No
  registration → no message index → no wire name → no format.
- `readelf -W -r reference/ts_i386.so | grep 1034ac` → **zero relocations**
  target the global. Nothing reads it, nothing writes it: never registered in
  `LinkUserMessages` (or anywhere), never used in a `MESSAGE_BEGIN`.
- `rz-bin -zz reference/client.dll | grep -i kickalign` → nothing; ditto
  `reference/mp.dll`. The **client has no hook and no handler** — consistent
  with the message never existing on the wire.
- Not present in `reference/HUD_MESSAGES.md` or `reference/API_SURFACE.md`
  (grep for `Kick`/`Align`: no hits) — the docs' silence corroborates the
  binary: this is not part of the external swap contract.

So the byte-faithful swap contract for `TSKickAlign` is: **the message must NOT
appear on the wire, must NOT be registered (no `REG_USER_MSG("TSKickAlign", …)`),
and must NOT be hooked client-side.** The only original artifact is the
declared-but-dead global variable.

## PURPOSE

Unknown / vestigial. The name suggests a planned server→client "kick alignment"
sync — most plausibly aligning the client's view-kick / recoil punch with the
server (cf. `CTSGun::VKickback(float, int, bool, float, float, float)` and
`DropKickback(float*, float*, float, float)` in `reference/signatures-gcc2.txt`,
the only "kick"-named functions in the binary). It was evidently abandoned
before shipping (or its use was compiled out), leaving only the global.
**VERIFY**: the purpose inference is name-based only — there is nothing in
either binary to confirm it, and for parity purposes it does not matter (the
feature has no runtime footprint).

## SERVER SEND SITES (+VERIFY flags)

**None.** Zero relocations against `0x1034ac` in `ts_i386.so` means no code in
the shipped server ever touches the global — no registration site, no send
site. `reference/coverage/AUDIT.md:158` lists its source file as `?` and
`ts_inventory.json` has `"file": null` for the same reason: the file
attribution is reference-based, and nothing references it.

Placement hint (for choosing where WE declare it): in the original `.data`
layout it sits in the contiguous gmsg block at `0x103400–0x1034d4`, between
`gmsgTSWires` (0x1034a8, attributed `TS_player.cpp`) and `gmsgTSServerSetting`
(0x1034b0, attributed `TS_gamerulesclass.cpp`). The whole block interleaves
attributions, suggesting the gmsg ints were declared together in one TU
(classic-SDK style) and the per-file attribution merely reflects who USES each.
**VERIFY** (re-project pass, low priority): which TU declared the block — it
only affects our cosmetic placement, not behaviour or the audit (audit cannot
check a `null` file).

## CLIENT HANDLER BEHAVIOR

**None.** No `"TSKickAlign"` string in `reference/client.dll`, so no
`HOOK_MESSAGE`/`pfnHookUserMsg` for it and no `MsgFunc_*` handler. It drives no
HUD or client state. **VERIFY** is not needed here — absence of the hook string
is conclusive (a hooked message requires its name string in the client binary).

## OURS

**Absent — and that is (almost) correct.** `grep -rn "KickAlign"` over `dlls/`
and `cl_dll/` finds nothing. We did NOT invent a differently-named substitute:
the audit's invented globals serve other purposes (`gmsgStatusText`/
`gmsgStatusValue` = status-bar text in `dlls/player.cpp:1906/1935`;
`gmsgWeapons` = weapon-bits resend in `dlls/player.cpp:4421`) and none carries
recoil/kick data. Our recoil (VKickback) is implemented without any user
message, matching the original's shipped behaviour.

The only gap is **source parity**: the original binary exports the global; our
build does not have it, so `audit.py` reports it MISSING (`AUDIT.md:158`).

## RECONCILE

This is the `gmsgSetCurWeap` pattern (declared in `dlls/UserMessages.h:45`,
never registered in `LinkUserMessages`): **declare the global, register
nothing, send nothing, hook nothing.**

1. **Server: add the dead global only.**
   `int gmsgTSKickAlign = 0; // DEAD in original: never registered/sent (no name string, no relocs) — parity only`
   Best-fit home given our per-file convention: `dlls/ts_player.cpp` next to the
   other TS gmsg definitions (its .data neighbours), or `dlls/UserMessages.h`
   alongside `gmsgSetCurWeap`. Either satisfies the audit (original file is
   `null`); it must have external linkage (non-static) so it appears in our
   symbol table for `audit.py` to match by name.
2. **Do NOT add `REG_USER_MSG("TSKickAlign", …)`** — that would register a
   message the original never registers, changing the message-index sequence
   handed out by the engine and adding a name the original client never hooks.
   That is a swap-contract violation, not a fix.
3. **Client: nothing.** No hook, no handler, no HUD state.
4. **No existing code absorbs it** — there is no behaviour to absorb; the
   reconcile is purely the declared global.
