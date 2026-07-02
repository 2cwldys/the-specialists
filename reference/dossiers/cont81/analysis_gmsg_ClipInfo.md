# gmsgClipInfo — "ClipInfo" user message analysis

## WIRE FORMAT

```
ClipInfo (registered size 1, fixed):
  BYTE  clip     // loaded rounds of the player's CURRENT weapon
```

- Registration (binary-proven, `ts_i386.so`): inside `LinkUserMessages__Fv` @0xc7db0,
  at 0xc8145: `push $0x1; lea "ClipInfo"(0xFE4BE); call REG_USER_MSG; mov result -> gmsgClipInfo`
  (GOT slot 0x167bf0 -> global `D gmsgClipInfo` @0x10345c).
- Registration ORDER in the TS block: `WeaponInfo`(6) -> **`ClipInfo`(1)** -> `WStatus`(3) ->
  `ActItems`(2) -> `KFuPower`(1) -> ... (matches HUD_MESSAGES.md table).
- Read side (client handler FUN_1004f780) does exactly one `READ_BYTE()` — consistent with the
  fixed size 1. No other fields.

## PURPOSE

Lightweight clip-count update for the weapon-select bar / weapon grid: it tells the client HUD
"the current weapon's loaded clip is now N" without resending the full 6-byte `WeaponInfo` or the
per-slot `WStatus`. In practice it is **vestigial**: the server registers it but never sends it
(clip actually rides in `WeaponInfo` byte 2, per API_SURFACE.md:40); the client keeps a live
handler anyway.

## SERVER SEND SITES

**NONE — definitively.** `readelf -W -r` shows exactly one relocation user of `gmsgClipInfo`
(GOT slot 0x167bf0, offset 0x1de0 from GOT base 0x165E10), and `objdump -d` over the whole .so
finds exactly one code reference to `0x1de0(%ebx)` — at 0xc8154, inside `LinkUserMessages__Fv`,
and it is the registration store, not a MESSAGE_BEGIN. So the original server **registers
"ClipInfo" with size 1 and never sends it**. No `ClipInfo`-related functions exist in
signatures-gcc2.txt; strings confirm only the registration literal (.rodata 0xfe4be) and the
symbol name.

- VERIFY (re-project pass, low priority): confirm the original global `gmsgClipInfo` is defined
  in `player.cpp` (AUDIT/funcfile claim) — affects only file-parity placement, not the wire.

## CLIENT HANDLER BEHAVIOR

Handler: `FUN_1004f780` (client.dll), hooked in the weapon-grid class Init @0x1004c9d0 via
forwarder 0x1004c820, `this` = static instance 0x100d0ca0 (the original CHudTSWeaponBar/grid).

```c
// MsgFunc_ClipInfo(this, name, iSize, pbuf)
BEGIN_READ(pbuf, iSize);                       // FUN_100299b0
this->m_iCurClip /*+0x78c*/ = READ_BYTE();     // FUN_10029a10
if (this->m_iCurWeaponType /*+0x7b8*/ != 0) {
    info = GetWeaponInfo(m_iCurWeaponType);    // FUN_100071c0 -> s_weapon_info record
    b85 = *(byte*)(info+0x85);                 // grid row/col byte (pair with +0x84;
    b84 = *(byte*)(info+0x84);                 //  cf. ts_weaponbar.h: +0x84 column, +0x85 row)
    // grid array @ this+0x168, entry stride 8 bytes, 11 entries per major index:
    *(int*)((char*)this + 0x168 + (b85*11 + b84)*8) = this->m_iCurClip;  // entry dword0 = clip
}
this->m_iFlags /*+0x10*/ |= 1;                 // HUD_ACTIVE — request redraw
```

Effect: updates (a) the scalar "current clip" member and (b) the current weapon's clip cell in
the per-weapon grid the bar draws from — the same array the `WStatus` handler populates. Since
the server never sends ClipInfo, this code never runs in stock play, but it must be HOOKED for
hook-list parity (REPORT.md flags it MISSING client-side).

## OURS

**Absent on both sides.** `grep -r ClipInfo dlls/ cl_dll/` -> zero hits.

- NOT a rename case: the invented globals are unrelated —
  `gmsgStatusText`/`gmsgStatusValue` are stock-HL train/objective status (`"StatusText"` -1,
  `"StatusValue"` 3), and `gmsgWeapons` (`"Weapons"`, 8, player.cpp:4421) is an invented 8-byte
  weapons sync — none is a 1-byte clip message. Nothing of ours substitutes for ClipInfo.
- Our functional equivalent of the ORIGINAL's grid-clip state already exists:
  `CHudTSWeaponBar::m_iClip[38]` (cl_dll/ts_weaponbar.cpp, fed by `WStatus`), and the active
  weapon's live clip comes via `CHudTSAmmo::MsgFunc_WeaponInfo` (byte 2). So behaviour parity is
  already there; only the message registration + hook are missing.

## RECONCILE

Server (dlls/):
1. Add `inline int gmsgClipInfo = 0;` to `dlls/UserMessages.h` and
   `gmsgClipInfo = REG_USER_MSG("ClipInfo", 1);` in `LinkUserMessages()`
   (dlls/UserMessages.cpp), placed right after the `WeaponInfo` registration to mirror the
   original order. (Original source file was player.cpp; our SDK centralizes registration in
   UserMessages.cpp — note for audit file-parity, wire-irrelevant since the engine transmits
   name->id mappings at connect.)
2. **Never send it.** No MESSAGE_BEGIN(gmsgClipInfo) anywhere — matching the original exactly
   (clip continues to ride in `WeaponInfo`).

Client (cl_dll/):
3. Hook it in the weapon bar: `HOOK_MESSAGE(ClipInfo)` in `CHudTSWeaponBar::Init()` +
   `bool CHudTSWeaponBar::MsgFunc_ClipInfo(const char*, int iSize, void* pbuf)`:
   `BEGIN_READ(pbuf, iSize); int clip = READ_BYTE();` then, if a current weapon type is known
   (gHUD's active tsweapon_t from WeaponInfo/CurWeapon), `m_iClip[curType] = clip;` and
   `m_iFlags |= HUD_ACTIVE;`. This is the faithful translation of the original grid write
   (original indexes by grid row/col; ours indexes m_iClip by weapon type — same state, our
   representation).
4. Re-run `scripts/coverage/coverage.py all` — the `ClipInfo` rows should flip from MISSING
   (server registration + client hook) to matched.
