# gmsgSelAmmo — "SelAmmo" user message analysis

## WIRE FORMAT

- Registered: `REG_USER_MSG("SelAmmo", 4)` — fixed **4 bytes**.
- Verified in the binary: `LinkUserMessages__Fv` @ file 0xc7db0 in `ts_i386.so`
  pushes `$0x4` + `lea` of `.rodata` string "SelAmmo" (0xfe3b2) into
  `enginefuncs+0x12c` (pfnRegUserMsg) and stores the id into `gmsgSelAmmo`
  (.data 0x1033cc, GOT slot 0x167f14). It is the **FIRST** message registered.
- Layout = the stock HL SDK `SelAmmo` struct (declared in the 2001 SDK's
  `cdll_dll.h`), never actually written by any sender in TS:
  ```
  BYTE Ammo1Type;   // primary ammo type index
  BYTE Ammo1;       // primary ammo count
  BYTE Ammo2Type;   // secondary ammo type index
  BYTE Ammo2;       // secondary ammo count
  ```
  Since no send site exists, the struct layout is provenance-only (stock SDK),
  not binary-verified — but the **registered name + size 4** IS binary-verified
  and is the swap contract.

## PURPOSE

Stock Half-Life SDK leftover. In the 2001 SDK it was intended to tell the
client which ammo types/counts the selected weapon uses; Valve's own SDK
registers it and never sends it, and the stock cl_dll never hooks it. TS
inherited that verbatim: **register-only dead wire name**. Its only live role
in TS is (a) occupying the first user-message id slot and (b) serving as the
"already linked" guard variable in `LinkUserMessages`
(`if (*gmsgSelAmmo != 0) return;` — the first instructions of the function).

## SERVER SEND SITES

- **NONE.** Exactly one `.text` reference to the GOT slot in the whole `.so`:
  `objdump -d | grep '0x2104(%'` -> single hit at 0xc7dbf inside
  `LinkUserMessages__Fv` (0x2104 = 0x167f14 - GOT base 0x165E10). That
  reference is the guard-read + registration store described above.
- No `MESSAGE_BEGIN(..., gmsgSelAmmo, ...)` anywhere. mp.dll only carries the
  "SelAmmo" registration string in `.data` @ 0x100a110c (consistent).
- Original source file: `player.cpp` `LinkUserMessages()` (stock SDK
  placement). VERIFY (re-project pass): the exact guard expression and that TS
  kept the stock registration ORDER for everything before/after it — the
  HUD_MESSAGES.md order list (SelAmmo, CurWeapon, Geiger, Health, Damage,
  Battery, Train, HudText, SayText, TextMsg, WeaponList, ResetHUD, InitHUD,
  GameTitle, DeathMsg, ScoreInfo, TeamInfo, TeamScore, GameMode, MOTD,
  ServerName, AmmoPickup, WeapPickup, ItemPickup, HideWeapon, SetFOV,
  ScreenShake, ScreenFade, AmmoX, TeamNames, then the TS block) already
  encodes it; note the original list has **no Flashlight/FlashBat** (we
  register those — separate audit item, not SelAmmo's).

## CLIENT HANDLER BEHAVIOR

- **No handler.** `rz-bin -zz reference/client.dll | grep -i selammo` -> zero
  hits; the original TS client.dll never calls
  `HOOK_MESSAGE(SelAmmo)`/`gEngfuncs.pfnHookUserMsg("SelAmmo", ...)`.
  (Stock HL cl_dll doesn't hook it either.) An unhooked user message is simply
  parsed-and-skipped by the engine using the registered size (4) — which is
  exactly why the registered size must match. VERIFY flag: n/a — absence is
  established by the string scan; nothing reader-side to recover.

## OURS

- **Absent entirely.** `grep -rn SelAmmo dlls/ cl_dll/ common/` -> zero hits.
  halflife-updated (our base SDK) deleted the dead registration; our
  `dlls/UserMessages.cpp` `LinkUserMessages()` starts at
  `gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3)` and guards on
  `gmsgCurWeapon` instead of `gmsgSelAmmo`.
- The audit's invented globals are NOT renamed substitutes for SelAmmo:
  - `gmsgStatusText` (`"StatusText"`, -1) / `gmsgStatusValue`
    (`"StatusValue"`, 3) — stock HL train/objective status HUD, different
    names/sizes/purpose; original TS registers neither (they are EXTRA and a
    separate reconcile item).
  - `gmsgWeapons` (`"Weapons"`, 8) — our invented arsenal-sync message
    (dlls/player.cpp:4421), 8 bytes, unrelated to a 4-byte ammo-type struct.
  SelAmmo carries no traffic in the original, so by definition nothing of ours
  can be a functional substitute. oursStatus = absent.

## RECONCILE

Server side only; register-only, byte-faithful:

1. `dlls/UserMessages.h`: add `inline int gmsgSelAmmo = 0;`.
2. `dlls/UserMessages.cpp` `LinkUserMessages()`:
   - add `gmsgSelAmmo = REG_USER_MSG("SelAmmo", 4);` as the **first**
     registration (before CurWeapon), matching the original order;
   - for exact parity, switch the already-linked guard to
     `if (0 != gmsgSelAmmo) return;` (the original guards on its
     first-registered message).
   (If the source-parity restructure moves registration back into
   `player.cpp` to match `funcfile.tsv`, the same two edits apply there.)
3. **No send site** — do not invent one. No `SelAmmo` struct needed unless we
   mirror the stock header for provenance; `4` literal (or
   `sizeof(SelAmmo)` with the stock 4-byte struct) both satisfy the contract.
4. Client side: **nothing** — do not hook it. The engine skips unhooked
   fixed-size messages using the registered size.
5. Existing code absorbing it: none needed; nothing to migrate or rename.
   Separately flagged (not this message's reconcile): our EXTRA
   Flashlight/FlashBat/ShowMenu/Spectator/StatusText/StatusValue/Weapons
   registrations vs the original list.

Evidence commands (all lock-free):
- `nm -D reference/ts_i386.so | grep SelAmmo` -> `001033cc D gmsgSelAmmo`
- `readelf -W -r reference/ts_i386.so | grep SelAmmo` -> GOT 0x167f14
- `objdump -d -j .text reference/ts_i386.so | grep '0x2104(%'` -> 1 hit
  (LinkUserMessages @0xc7dbf)
- `rz-bin -zz reference/client.dll | grep -i selammo` -> none
