# ★ SrvSett — InitHUD + Think change-detect + weaponrestriction (SETTLED, with 2 dossier corrections)

Ground truth: typed decompiles of `CTSGameRules::InitHUD` @ nm **0x69134** (`star_decomp.txt`),
`CTSGameRules::Think` @ **0x67204**, `cmd_weaponrestriction` @ **0xb58a0**,
`display_weaponrestriction` @ **0xb5770** (`star_decomp2.txt`); cvar_t structs read from .data;
GOT scan disp 0x1f34 = exactly 6 refs: registration + Think ×3 + InitHUD + cmd_weaponrestriction
(no other senders exist).

## ⚠ CORRECTION 1 — setting id 0 is **realbullet**, NOT roundtime

Think's id-0 change-detect reads cvar_t global `realbullet` (GOT 0x167544 → .data 0x108efc,
**registered cvar name `"realbullet"`, default `"0.0"`**). The dossier (and the client scout's
"g_roundtimeSetting" guess for DAT_1018d16c) had id0 = roundtime — wrong. realbullet is the
"bullets are real projectiles" mode (CBulletManager), which the client fire path needs.

## ⚠ CORRECTION 2 — id 1's C variable is `dmcashbased`, whose cvar NAME is `"usecash"`

The .so symbol `dmcashbased` @0x108f10 is the cvar_t whose registered name string is **"usecash"**
(default "0.0"). So: our `usecash` cvar registration is name-correct; the defining variable, for
audit/source parity, should be `dmcashbased`.

## InitHUD (MSG_ONE to the joining player) — sends ALL FOUR ids (not just id 0)

```c
// (preceded by: if cheats!=0 → green TSHudMessageOne "This server has cheats enabled."
//  and, still under cheats!=0, if damagemult.value>0 → "This server has custom weapon damages (%.2f)")
// if (this->field_0x5884) InformClientsOnWeaponsParams(player);   // CustomWP gate confirmed
MSG_ONE SrvSett: BYTE 0, SHORT this->field_0x578;                  // cached (int)round(realbullet)
MSG_ONE SrvSett: BYTE 1, SHORT (dmcashbased.value != 0);           // usecash cvar, read DIRECT
MSG_ONE SrvSett: BYTE 2, SHORT (int)round(cv_ammocount.value);     // ammocount cvar, read DIRECT
MSG_ONE SrvSett: BYTE 3, SHORT this->field_0x5870;                 // weaponrestriction bitmask
```
- **No late-joiner hole**: the dossier's map-change worry is moot — InitHUD pushes ids 0–3 to
  every joiner. InitHUD does NOT call `Set_ammo_count` (only Think does, on change).
- Wire format confirmed: BYTE id + SHORT value = 3 bytes fixed.

## CTSGameRules::Think — 3 on-change MSG_ALL sends (tail of Think, every frame)

```c
if ((float)field_0x578 != realbullet.value) {         // cache @rules+0x578 (int, holds round())
    field_0x578 = (int)round(realbullet.value);
    MSG_ALL SrvSett: BYTE 0, SHORT field_0x578; }
// (between id0 and id1: if field_0x5864 → CBulletManager::Think(&field_0x1864))
if ((float)field_0x558 != dmcashbased.value) {        // cache @rules+0x558
    field_0x558 = (int)round(dmcashbased.value);
    MSG_ALL SrvSett: BYTE 1, SHORT (field_0x558 != 0); }   // sent as 0/1 bool
v = (int)round(cv_ammocount.value);
if (v != field_0x55c) {                               // cache @rules+0x55c
    field_0x55c = v;
    Set_ammo_count(v);                                // shared fn, called BEFORE the send
    MSG_ALL SrvSett: BYTE 2, SHORT field_0x55c; }
```
- MSG_ALL (dest=2) for all three; change-detect member offsets confirmed: 0x578 / 0x558 / 0x55c.
- id1's cached value is the ROUNDED cvar but the WIRE value is the bool `!= 0`.
- Rounding = FPU round-to-nearest (`(int)ROUND(...)`) on all cvar reads.

## cmd_weaponrestriction (game.cpp) — id 3

```c
if (Cmd_Argc() < 2) { display_weaponrestriction(); return; }
old = rules->field_0x5870;
if (atoi(Cmd_Argv(1)) == 5) rules->field_0x5870 = InitWeaponRestriction();
else rules->field_0x5870 = WeaponRestrictionByID(atoi(Cmd_Argv(1))) & rules->field_0x586c;
if (rules->field_0x5870 != old)                        // send ONLY on change
    MSG_ALL SrvSett: BYTE 3, SHORT rules->field_0x5870;
display_weaponrestriction();                           // always printed
```
- The value is a category **bitmask** masked by rules+0x586c (the allowed-categories mask);
  argv "5" = reset to InitWeaponRestriction() (= all).
- `display_weaponrestriction` text format: if mask==0 → `ALERT(at_console, "All weapons are
  restricted.\n")`; else strcat of `g_szRestrictions[1..5]` for bits 1/2/4/8/0x10 →
  `"Weapons allowed: %s\n"`. `g_szRestrictions[]` (@.data 0x109034) = `{"Kung Fu ", "Knives ",
  "Pistols ", "SMGs ", "Rifles ", "Shotguns "}` (index 0 "Kung Fu " never printed — bit0's
  category is index 1 "Knives"; i.e. Kung Fu is always allowed).

## Registration & client
`REG_USER_MSG("SrvSett", 3)` between `Wires` and `TSPAward` (star_LinkUserMessages.md #51).
Client handler unchanged from the dossier (4-way switch; case 2 → shared `Set_ammo_count`), but
rename the client globals: DAT_1018d16c = realbullet setting (not roundtime), DAT_1018d170 =
usecash bool, DAT_1018d174 = weaponrestriction bitmask.
