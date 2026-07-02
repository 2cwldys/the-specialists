# gmsgSetCurWeap — analysis (mod-ts drop-in parity)

Verdict up front: **DEAD GLOBAL — no wire format exists, nothing to implement.**
`gmsgSetCurWeap` is a stock vanilla-HL-SDK leftover variable that is declared but
never registered, never assigned, never sent, and never hooked client-side, in the
original TS binaries AND in our tree. Parity is already exact.

## WIRE FORMAT

**None — the message never exists on the wire.**

Binary evidence (all lock-free queries against `reference/ts_i386.so` / `reference/client.dll`):

- `nm reference/ts_i386.so` → `00103434 D gmsgSetCurWeap` — the symbol exists in `.data`.
- `readelf -W -r reference/ts_i386.so | grep gmsgSetCurWeap` → **0 relocations**.
  (Contrast: `gmsgCurWeapon` @0x1033e4 has a relocation — it is really used.)
  In PIC i386 code every access to a global goes through a relocation; zero relocs
  means **no instruction in the .so ever reads or writes this variable**. So it is
  never passed to `REG_USER_MSG` and never used in a `MESSAGE_BEGIN`.
- `rz-bin -zz reference/ts_i386.so | grep -i curweap` → the string `"SetCurWeap"`
  appears **only** in `.dynstr`/`.strtab` (i.e., as the *variable's symbol name*,
  `gmsgSetCurWeap`). There is **no** `"SetCurWeap"` message-name string in `.rodata`,
  where every real registered message name lives (`"CurWeapon"` IS there, @0xfe3ba).
- `rz-bin -zz reference/client.dll | grep -i curweap` → only `"CurWeapon"` (@.data
  0x100930ec). No `"SetCurWeap"` → the client never calls
  `HOOK_MESSAGE(SetCurWeap)` / `pfnHookUserMsg("SetCurWeap", …)`.
- `reference/API_SURFACE.md` "Full hooked list (verbatim)" (76 messages the client
  hooks): `SetCurWeap` is **absent**. `CurWeapon` is present (stock, 3 bytes).
- `reference/HUD_MESSAGES.md` documents `CurWeapon 3` among stock messages; no
  `SetCurWeap` entry anywhere.

## PURPOSE

Historical only. `int gmsgSetCurWeap = 0;` is a **vanilla Half-Life SDK dead
global** (declared in stock `dlls/player.cpp` alongside the other `gmsg*` ints,
never registered even in Valve's own SDK). TS inherited it by building on the HL
SDK and never used it either. Its *intended* purpose (per the name, in the
never-shipped Valve design) would have been "force-set the client's current
weapon", but the actual current-weapon HUD state is carried entirely by the stock
`CurWeapon` message (3 bytes: `BYTE state, BYTE weapon-id, BYTE clip`), which TS
uses normally. It drives **no** HUD or state.

Provenance note for AUDIT.md:154 (`original file '?'`): the stock SDK declares
this global in **`player.cpp`** — same file as its dead siblings in that AUDIT
table region. Our own upstream base (`upstream/64bit-demo`
`dlls/UserMessages.h:45`) carries the identical declaration, which is exactly
where ours comes from.

## SERVER SEND SITES

**None.** Zero relocations against the symbol means there are no send sites and
no registration site in `ts_i386.so`; it is not in `LinkUserMessages`' registered
set (no `"SetCurWeap"` `.rodata` string for `REG_USER_MSG` to take).

- `grep SetCurWeap reference/signatures-gcc2.txt` → no function references it
  (only `LinkUserMessages()` itself exists at line 1555, as expected).
- VERIFY (re-project pass, low priority): confirm in the typed Ghidra `re`
  project that `LinkUserMessages`' decompile registers no `"SetCurWeap"` — the
  static string/reloc evidence above already makes this a near-certainty; a
  decompile read is just belt-and-braces.

## CLIENT HANDLER BEHAVIOR

**None.** No `"SetCurWeap"` string in `client.dll` → no hook, no handler, no
`MsgFunc_SetCurWeap`. (Flagged VERIFY only in the formal sense that no client
decompile was run — the absence of the hook-name string is conclusive, since
`pfnHookUserMsg` requires the literal name string.)

## OURS

**Exact parity already.**

- `dlls/UserMessages.h:45`: `inline int gmsgSetCurWeap = 0;` — declared, never
  registered (`grep SetCurWeap dlls/UserMessages.cpp` → nothing; only
  `gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3)` at line 30).
- No send site: `grep -rn SetCurWeap dlls/ cl_dll/` hits only the header line.
- Not a wrong-name substitute situation: the audit's invented globals
  `gmsgStatusText`/`gmsgStatusValue` correspond to the **stock** `StatusText`/
  `StatusValue` messages, which ARE in the original's hooked list — they are not
  renames of `SetCurWeap`. Since `SetCurWeap` has no wire presence at all, no
  code of ours could be substituting for it.
- This exactly mirrors the upstream stock SDK (`upstream/64bit-demo`
  `dlls/UserMessages.h:45` has the same line), which mirrors the original.

## RECONCILE

**No action.**

- Do NOT register it, do NOT send it, do NOT hook it client-side — doing any of
  those would *break* drop-in parity (the original client has no handler; the
  original server never emits it; registering it server-side would also shift
  nothing on the wire but would diverge the registered-message set).
- Keep the dead declaration `inline int gmsgSetCurWeap = 0;` in
  `dlls/UserMessages.h` as-is — it matches the original binary's dead
  `.data` global and the stock-SDK provenance.
- AUDIT.md housekeeping suggestion (for the audit tooling, not this pass): the
  `gmsgSetCurWeap` row's original file can be resolved from `?` to `player.cpp`
  (stock SDK), and the row is satisfied by our existing declaration — audit.py's
  `.h`-scanning blind spot / global-matching may be why it shows MISSING; worth
  a one-line check that audit matches `inline` header globals.
