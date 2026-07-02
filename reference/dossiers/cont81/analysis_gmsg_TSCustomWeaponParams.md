# gmsgTSCustomWeaponParams — wire name "CustomWP"

Server global: `gmsgTSCustomWeaponParams` (ts_i386.so `D 0x1034c4`; name string "CustomWP" @ .rodata 0xfe55b).
Registered `REG_USER_MSG("CustomWP", -1)` (variable size — HUD_MESSAGES.md line 45 confirms `CustomWP | -1`).

## WIRE FORMAT (byte-faithful swap contract)

Body = **one WRITE_STRING** of a sprintf'd line, format string `"%i %f %f %f %f %i"`
(server .so .rodata @0xf9d50; identical copy in client.dll @0x10095edc). Fields, in wire order,
with the `s_weapon_info` field each maps to (offsets are the ORIGINAL 988-byte struct):

| # | fmt | value | orig offset | our field (ts_weapon_info.h) |
|---|-----|-------|-------------|------------------------------|
| 1 | %i | tsweapon_t weapon id (0..0x25) | — (table index) | tsweapon_t |
| 2 | %f | base spread | wi+0x28 | `flSpread` |
| 3 | %f | view kick | wi+0x30 | `flViewKick` |
| 4 | %f | recoil (per-shot spread increment) | wi+0x34 | `flRecoil` |
| 5 | %f | per-bullet damage | wi+0x1c | `flDamage` |
| 6 | %i | buy-menu price | wi+0xd8 | **MISSING in ours** (`iPrice`) |

The +0xd8 = price identification is CONFIRMED: every `Write_<weapon>` initializer stores a
buy-price-shaped constant there (knife 0x64=100, skorpion 0x352=850, barrett 0x2328=9000,
M60 0x1388=5000, ...), and the client handler stores field 6 to wi+0xd8.

Both sides confirmed by disassembly:
- **Server writer** `SendWeaponParams(tsweapon_t, CBaseEntity*)` @0x8eb98: sprintf push order
  (cdecl, floats promoted to double) = `(buf, fmt, w, [+0x28], [+0x30], [+0x34], [+0x1c], [+0xd8])`.
- **Client parser** FUN_1004fb50 sscanf vararg stack-slot analysis: out2→wi+0x28, out3→wi+0x30,
  out4→wi+0x34, out5→wi+0x1c, out6→wi+0xd8. Exact match.

Routing: `pEntity != NULL` → `MESSAGE_BEGIN(MSG_ONE, gmsgTSCustomWeaponParams, NULL, pEntity->pev->pContainingEntity)`;
`pEntity == NULL` → `MSG_ALL` (engine call `*0xb8` MessageBegin, `*0xd8` WriteString, `*0xbc` MessageEnd).
Guards in SendWeaponParams: `w > 0x25` → no-op; `get_weapon_info(w)` entry byte-0 empty (unused slot) → no-op.
Local sprintf buffer ~68 bytes with a forced NUL at buf+0x43; client copies the READ_STRING into a 64-byte local.

## PURPOSE

The **custom-weapons sync** message. TS lets server operators override per-weapon combat
parameters (spread / view-kick / recoil / damage / price) via the `customweapons` cvar
(points at a file, e.g. `weapons_custom.txt`) and the `loadweapons` / `saveweapons` server
commands. Because CTSGun is **client-predicted** (fire.sc events compute spread/kick from the
client's mirror of `s_weapon_info`) and the buy menu prices come from the same table, the
client's copy MUST be updated to match the server's — this message carries one weapon's
overridden row. Without it, a custom server would show wrong prices and mispredict fire.

## SERVER SEND SITES

All in the original `TS_util.cpp` region (0x8ea3c–0x8ec9x), confirmed by objdump:

1. **`SendWeaponParams(tsweapon_t, CBaseEntity*)` @0x8eb98** — the single writer (above).
2. **`InformClientsOnWeaponsParams(CBaseEntity*)` @0x8eb14** — full-table sync:
   - first sends a HUD notice, string @0xfa1e0 `"This server is using custom weapons."`:
     `pEntity ? TSHudMessageOne(45,195,45, 8, 100, str, pEntity->pev) : TSHudMessageAll(45,195,45, 8, 100, str)`
     (RGB 45,195,45 green; params 8,100 = channel/hold VERIFY exact meaning);
   - then loops ALL weapon ids 0..0x25 (unrolled x2) calling `SendWeaponParams(i, pEntity)`.
   Callers (objdump PLT xrefs — complete):
   - **`CTSGameRules::InitHUD(CBasePlayer*)` @0x691d8** — gated on the byte flag
     `CTSGameRules+0x5884` ("custom weapons active"): each JOINING player gets the whole
     table MSG_ONE. (VERIFY in re-project pass: exact member home of +0x5884 — read via
     `this` in InitHUD, via a GOT-slot global in AdjustSpecificWeapon.)
   - **`cmd_loadweapons()` @0xb59ed** — after (re)loading the customweapons file →
     broadcast (VERIFY: presumably passes NULL → MSG_ALL; and whether it also sets +0x5884).
3. **`AdjustSpecificWeapon(tsweapon_t, float, float, float, float, int)` @0x8ea3c** — the
   live-tweak path: writes the 5 values into the table (+0x28/+0x30/+0x34/+0x1c/+0xd8 in that
   arg order = wire order), `SendWeaponParams(w, NULL)` (MSG_ALL),
   `UTIL_ClientPrintAll(3, UTIL_VarArgs("%s behaviour changed", wi->name@+0x44))`
   (fmt @0xf9d3a), and sets the +0x5884 custom-weapons flag = 1.
   Likely invoked from `adjust_weapon_value(char*)` @0x8eca4 (VERIFY caller chain + the
   console command that reaches it — no direct static call sites to AdjustSpecificWeapon found).
4. Related (VERIFY for the customweapons subsystem, not this message): `cmd_saveweapons()`
   @0xb5a2c writes the file — per-line format `" %i %f %f %f %f %i "` @0xf9d8c (same fields,
   space-padded); file header comment @~0xfa250 mentions
   `//If you want use different weapon settings use the 'customweapons' cvar.`

## CLIENT HANDLER BEHAVIOR (ground truth: FUN_1004fb50 @ client.dll)

`__thiscall` member of the buy-menu/weapon-grid object (instance @0x100d0ca0, forwarder
@0x1004c8c0), signature `(pszName, iSize, pbuf)`, `ret 0xc`:

1. `BEGIN_READ(pbuf, iSize)`; `READ_STRING()`; copy into 64-byte local buffer.
2. `sscanf(buf, "%i %f %f %f %f %i", &id, &spread, &kick, &recoil, &dmg, &price)`.
3. `wi = get_weapon_info(id)` (client mirror FUN_100071c0, table DAT_100aa050) — then
   `wi+0x28 = spread; wi+0x30 = kick; wi+0x34 = recoil; wi+0x1c = dmg; wi+0xd8 = price`
   (re-fetches the pointer before every store, exactly as the server's Adjust does).
4. `this->field_0x10 |= 1` — a dirty/refresh flag on the buy-menu object (regenerate the
   grid/prices next open). Returns 1.

No bounds check beyond what get_weapon_info does internally (id outside [0,0x25] → returns
&table[0] per our RE of get_weapon_info @0x6f190 — so a bad id silently mutates slot 0; mimic,
don't "fix", for drop-in parity).

## OURS

**Absent — no substitute.** Verified:
- No `CustomWP` / `TSCustomWeaponParams` / `SendWeaponParams` / `InformClientsOnWeaponsParams` /
  `AdjustSpecificWeapon` / `adjust_weapon_value` / `loadweapons` / `saveweapons` anywhere in
  `dlls/` or `cl_dll/`.
- The audit's invented globals are NOT renamed versions of this message:
  `gmsgWeapons` ("Weapons", 8) = stock halflife-updated 64-bit weapon-bits inventory
  (player.cpp:4421); `gmsgStatusText`/`gmsgStatusValue` = stock SDK status-bar. All three are
  SDK leftovers to be handled separately — none carries weapon params.
- We DO already register the **`customweapons` cvar** (dlls/ts_cvars.cpp:40,
  `{"customweapons", "", FCVAR_SERVER}`) but it is inert — nothing reads it.
- Our `s_weapon_info` (dlls/ts_weapon_info.h) has the 4 float fields (flSpread/flViewKick/
  flRecoil/flDamage) but **no `iPrice` (+0xd8)**; the nearby `slotCost` is +0xd0, a different
  field. Our client buy menu (cl_dll/ts_buymenu.cpp) uses its own **hardcoded local price
  table** (`int price` in its struct), disconnected from the shared table — so even our own
  server couldn't push price changes to it today.

## RECONCILE

Server side (new code lives in our TS_util.cpp equivalent; names EXACTLY as original):
1. `dlls/UserMessages.h`: `inline int gmsgTSCustomWeaponParams = 0;`
   `dlls/UserMessages.cpp`: `gmsgTSCustomWeaponParams = REG_USER_MSG("CustomWP", -1);`
   (wire name **CustomWP**, size **-1** — the drop-in contract).
2. Add `int iPrice; // s_weapon_info+0xd8` to `s_weapon_info`; populate per weapon in
   `compile_weapons_info` from the Write_<weapon> constants (extend
   `scripts/extract_weapons.py` to pull +0xd8; spot-checked: melee/knife 100, skorpion 850,
   barrett 9000, M60 5000).
3. Implement `void SendWeaponParams(tsweapon_t w, CBaseEntity* pEntity)`:
   guard `w > 0x25` and empty table slot; `char buf[68]; sprintf(buf, "%i %f %f %f %f %i",
   w, wi->flSpread, wi->flViewKick, wi->flRecoil, wi->flDamage, wi->iPrice);`
   `MESSAGE_BEGIN(pEntity ? MSG_ONE : MSG_ALL, gmsgTSCustomWeaponParams, NULL,
   pEntity ? pEntity->edict() : NULL); WRITE_STRING(buf); MESSAGE_END();`
   NOTE: keep `%f` sprintf (6-decimal default) — the ASCII bytes are the wire format.
4. Implement `void InformClientsOnWeaponsParams(CBaseEntity* pEntity)`: TSHudMessage
   (45,195,45, 8, 100, "This server is using custom weapons.") One/All, then
   `for (i = 0; i <= 0x25; i++) SendWeaponParams((tsweapon_t)i, pEntity);`.
5. Implement `void AdjustSpecificWeapon(tsweapon_t w, float spread, float kick, float recoil,
   float dmg, int price)`: same guards; store the 5 fields; `SendWeaponParams(w, NULL)`;
   `UTIL_ClientPrintAll(3, UTIL_VarArgs("%s behaviour changed", wi->pszName))`; set the
   custom-weapons flag.
6. `CTSGameRules`: add the custom-weapons-active flag (provenance +0x5884, e.g.
   `m_iCustomWeapons` byte); in `InitHUD(CBasePlayer* pl)` after the existing HUD messages:
   `if (flag) InformClientsOnWeaponsParams(pl);`.
7. Follow-up work items (same subsystem, VERIFY pass with re.sh once the Ghidra lock frees):
   `cmd_loadweapons` / `cmd_saveweapons` / `adjust_weapon_value(char*)` + reading the
   `customweapons` cvar at rules init (file line format `" %i %f %f %f %f %i "`), and the
   exact command that drives adjust_weapon_value.

Client side:
8. Hook `"CustomWP"` from the buy-menu HUD element's Init (the original handler is a member
   of the weapon-grid/buy-menu object): `HOOK_MESSAGE(CustomWP);` →
   `MsgFunc_CustomWP(pszName, iSize, pbuf)`: BEGIN_READ, READ_STRING, sscanf
   `"%i %f %f %f %f %i"`, write the 5 fields into the SHARED `g_weaponInfo[id]`
   (we already share one TU — the prediction side then picks the values up for free),
   set the buy-menu dirty flag so it rebuilds, `return 1`.
9. Rewire `cl_dll/ts_buymenu.cpp` to read prices from `get_weapon_info(w)->iPrice`
   (dropping/deriving its local `price` field) so a CustomWP price update actually shows.
   Mimic the original's no-clamp behaviour (out-of-range id → slot 0 via get_weapon_info).
