# gmsgTSPlayerAward — wire name "TSPAward" (size 2)

Sources: client handler decompile `gmsg_TSPlayerAward.c` (FUN_1004b790, forwarder 0x10049a20,
this = CTSHud @0x100d43cc), `reference/HUD_MESSAGES.md:41`, `reference/API_SURFACE.md:47`,
`nm -D ts_i386.so` (`gmsgTSPlayerAward` @ 0x1034b4, D), `rz-bin -zz` ("TSPAward" in .rodata @0xfe541).

## WIRE FORMAT

Registered: `REG_USER_MSG("TSPAward", 2)` — fixed size 2 bytes.

| # | Type | Field | Notes |
|---|------|-------|-------|
| 1 | BYTE | entindex | player entity index, valid range 1..64 (handler checks `0 < v && v < 0x41`) |
| 2 | BYTE | award id | server player member `[0x1d3]`; client stores it as a byte |

Broadcast **MSG_ALL** (every client learns every player's award). Field order is
entindex first, award second — the handler reads them in that order with two
consecutive READ_BYTE calls (`FUN_10029a10`) after BEGIN_READ (`FUN_100299b0`).

Compact form: `"TSPAward", 2 — MSG_ALL: BYTE entindex (1..64), BYTE awardId`.

## PURPOSE

Per-player "point award" broadcast — API_SURFACE.md calls it the **point-award popup**.
It tells all clients which award (a small byte-sized id) player N most recently earned,
driving a CTSHud-drawn popup/per-player award display. Because it is MSG_ALL and keyed
by entindex, every client maintains the full per-player award table (usable for popups
about other players, scoreboard-adjacent display).

## CLIENT HANDLER BEHAVIOR (ground truth, FUN_1004b790)

```c
void __thiscall FUN_1004b790(CTSHud* this, name, iSize, pbuf) {
    BEGIN_READ(pbuf, iSize);
    uint idx   = READ_BYTE();
    uint award = READ_BYTE();
    if (0 < (int)idx && (int)idx < 0x41)                  // entindex 1..64
        *(char*)(&DAT_101901f4 + idx * 8) = (char)award;  // per-player table, stride 8
    this->m_iFlags |= 1;                                  // (+0x10) HUD_ACTIVE
}
```

- `DAT_101901f4` is a **per-player extra-info array with stride 8**, indexed directly by
  entindex (not entindex-1); the award byte sits at offset 0 of each 8-byte record. The
  sibling MSG_ALL message `ActItems` (BYTE entindex, BYTE activeItems) very likely writes
  a different offset of the same records — VERIFY when ActItems is analyzed.
- No validation of the award value itself; stored as a raw byte.
- Setting `m_iFlags |= HUD_ACTIVE` on the CTSHud element wakes its Draw, which renders
  the popup. The Draw-side presentation (sprite/text per award id, fade time, whether it
  only shows the LOCAL player's award) is **VERIFY** — decompile the CTSHud Draw chain
  (client project) in the re-project pass.

## SERVER SEND SITES (+VERIFY)

- Per HUD_MESSAGES.md: sent from **`CBasePlayer::UpdateClientData` @0x81d00** as MSG_ALL,
  entindex via the index-of-edict engine slot (GOT 0x19103c), award from player member
  `[0x1d3]`. Symbol `gmsgTSPlayerAward` confirmed in ts_i386.so (.data 0x1034b4); wire
  string "TSPAward" confirmed in .rodata.
- **VERIFY (re-project pass, server decompile not available here):**
  1. Change-detection: UpdateClientData runs every frame; an unconditional MSG_ALL would
     flood — almost certainly gated on `[0x1d3] != cached-previous` (the standard TS
     UpdateClientData pattern, cf. PwUp/TSState). Confirm the cache member offset.
  2. What SETS `[0x1d3]` (the award producer): no `*Award*` function symbol exists in
     ts_i386.so beyond the stock `CGameScore::AwardToTeam` (signatures-gcc2.txt has no
     TS award fn), so the assignment is inline in some kill/gamerules/stunt path — find
     writers of player+0x1d3 via `re.sh refs`/Ghidra when the lock frees.
  3. Whether the id resets to 0 after a delay, and the award-id enum values.
  4. Map-change: per the map-change-state-reset gotcha, confirm whether the original
     re-sends on respawn/level change (client table is per-map state).

## OURS

**Absent.** No "TSPAward"/"TSPlayerAward"/award-message code anywhere in `dlls/` or
`cl_dll/` (only unrelated hits: stock CGameScore in maprules.cpp, ts_dmhill frag awards,
ts_objectives cash — none send a user message with this shape).

The audit-flagged invented globals are **not** substitutes for this message:
- `gmsgStatusText`/`gmsgStatusValue` (UserMessages.cpp:68-69) = stock HL SDK messages;
  the original client hooks StatusText/StatusValue too (API_SURFACE "stock" row), so
  these are legitimate stock, not a renamed TSPAward.
- `gmsgWeapons` ("Weapons", 8, UserMessages.cpp:71) = a halflife-updated SDK addition,
  wire-shape totally different (8 bytes vs 2) and unrelated purpose. Separate audit item
  (probably should be removed for drop-in parity), but not this message.

So the reconcile is a clean ADD, not a rename.

## RECONCILE

Server (`dlls/`):
1. Declare `int gmsgTSPlayerAward` alongside the other TS gmsg globals (ts_player.cpp
   pattern) and register `gmsgTSPlayerAward = REG_USER_MSG("TSPAward", 2);` — keep the
   original LinkUserMessages ordering per HUD_MESSAGES.md (TS block: after SrvSett,
   before TSBTime).
2. Add a CBasePlayer byte member for the award id (provenance comment: player+0x1d3)
   plus a last-sent cache member.
3. In `CBasePlayer::TSUpdateClientData()` (ts_player.cpp:37), on change:
   `MESSAGE_BEGIN(MSG_ALL, gmsgTSPlayerAward); WRITE_BYTE(entindex()); WRITE_BYTE(m_iAward); MESSAGE_END();`
   Force a resend after level change/ResetHUD (map-change gotcha). VERIFY the exact
   gate + producer before wiring producers.

Client (`cl_dll/`):
4. `HOOK_MESSAGE(TSPAward)` in the CTSHud-equivalent element's Init; handler
   `MsgFunc_TSPAward(name,size,pbuf)`: READ_BYTE entindex, READ_BYTE award; bounds-check
   1..64; store into a per-player extra-info array (mirror the original's 8-byte-stride
   record — share it with ActItems when that lands); `m_iFlags |= HUD_ACTIVE`.
5. Draw side (the actual popup) is a separate VERIFY/RE task on the client Draw chain —
   the store+activate above is the byte-faithful contract half.

Absorption: no existing code absorbs it; nearest homes are ts_player.cpp
(TSUpdateClientData send block) server-side and the TS HUD element that will own the
per-player extra-info table client-side.
