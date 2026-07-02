# TS_gun.cpp free functions — weapon-param / ammo / spread / range-damage cluster

Group: 13 free functions, all `funcfile.tsv` → **TS_gun.cpp**. Ground truth = the ff_*.c
decompiles + targeted objdump on `reference/ts_i386.so` (GOT base %ebx = 0x165E10; Ghidra
addr = file addr + 0x10000).

Key globals (nm, all `b` bss):
- `global_weapon_info` @0x16db40 — the 38-entry s_weapon_info table, stride **0x3dc**.
- `global_ammo_info` @0x176e00 — **16-entry s_ammo_info table, stride 0x38 (14 dwords)**.
- `range_damage_multipler` @0x177180 — **float[10000]** falloff LUT.
- `spread_mul.1221` @0x16da40 — CalcSpread's function-local static.

## The recovered `s_ammo_info` struct (0x38 bytes) — offsets PROVEN by callers

| off | type | meaning | proof |
|---|---|---|---|
| +0x00 | float | ~0.1–0.5 per caliber — **VERIFY** (unreferenced in the sites read; weight/accuracy?) | compile_ammo_info |
| +0x04 | int | **price per magazine** (cash) | BuyAmmo @0x84289: `cash(player+0x540) - info[+4]`, negative → can't buy |
| +0x08 | int | **kind 1/2** (1 = pistol calibers, 2 = 5.56/7.62x39/50BMG/SHELLS/7.62x51) — likely the falloff-form selector, **VERIFY** | compile_ammo_info; cf. UpdateBullet gate on bulletrec+0x3c==2 |
| +0x0c | int | **max carry (reserve cap)** — the ONLY mode-scaled column, written by `Set_ammo_count` | CTSGun::AddAmmo @0x708ef/0x70930 caps `gun+0xd4+cal*4` against `global_ammo_info+cal*0x38+0xc`; CTSSniperAmmo::Spawn @0xb94d2 gives `info[+0xc]/2` |
| +0x10 | int | **rounds per magazine** (buy unit) | BuyAmmo @0x8431d: `AddAmmo(gun, cal, info[+0x10])`; CTSGroundAmmo::KeyValue `mags*info[+0x10]`; RestartRound compares vs it |
| +0x14 | int | 0x2000 = **8192** for all 13 — max bullet travel range (units) — **VERIFY** consumer | compile_ammo_info |
| +0x18 | float | 0..1 per caliber — **VERIFY** (penetration/pierce weight?) | compile_ammo_info |
| +0x1c | float | 1.0 for ALL 16 entries (runtime multiplier?) — **VERIFY** | init loop in compile_ammo_info |
| +0x20 | int | **full-damage range unit**: falloff index = dist/this (form B below) | UpdateBullet @0x5f24a + ShootBulletsPlayer @0x5fdc8: `fimull 0x20(%eax)` on `a_info.1134` |
| +0x24 | int | **bullet flight speed unit** (slow-mo bullets fly at 40×this) | our ts_bulletmgr RE (kept) |
| +0x28 | char[16] | caliber display name | string copies |

### The full table, CALIBER-ordered (entry index = caliber = get_ammo_info arg)

cal: name | +0 | price | kind | maxcarry mode0/1/2/3 | mag | travel | +0x18 | range | speed

- 1: "9 x 19mm"    | 0.4 | 50  | 1 | 210/90/30/0  | 30 | 8192 | 0.2  | 80  | 330
- 2: ".45Acp"      | 0.4 | 60  | 1 | 175/50/25/0  | 12 | 8192 | 0.2  | 90  | 280
- 3: "SHELLS"      | 0.3 | 120 | 2 | 60/32/16/0   | 8  | 8192 | 0.0  | 90  | 350
- 4: "5.56 Nato"   | 0.3 | 80  | 2 | 120/60/30/0  | 30 | 8192 | 0.9  | 150 | 900
- 5: ".50AE"       | 0.3 | 70  | 1 | 70/21/7/0    | 10 | 8192 | 0.5  | 90  | 480
- 6: "7.62 x 39mm" | 0.2 | 100 | 2 | 90/60/30/0   | 30 | 8192 | 0.7  | 170 | 790
- 7: "5.7 x 28"    | 0.5 | 120 | 1 | 160/40/20/0  | 20 | 8192 | 1.0  | 100 | 730
- 8: "50BMG"       | 0.1 | 500 | 2 | 30/10/5/0    | 5  | 8192 | 1.0  | 200 | 1000
- 9: "10mm AUTO"   | 0.3 | 70  | 1 | 90/30/15/0   | 15 | 8192 | 0.5  | 90  | 450
- 10: ".22 LR"     | 0.5 | 100 | 1 | 150/34/17/0  | 17 | 8192 | 0.9  | 70  | 350
- 11: ".454 Casull"| 0.3 | 150 | 1 | 50/15/5/0    | 5  | 8192 | 0.75 | 20  | 400
- 12: ".32Acp"     | 0.5 | 30  | 1 | 300/100/40/0 | 20 | 8192 | 0.0  | 40  | 300
- 13: "7.62 x 51mm"| 0.2 | 150 | 2 | 35/15/5/0    | 5  | 8192 | 0.7  | 170 | 850

Entries 0/14/15: all zero except +0x1c = 1.0. **Names**: rodata @file 0xF7FF8.. — note cal13
is **7.62x51mm** (M60 belt), NOT ".45LC" as our comments say; cal6 is 7.62x39 (AK).

### ⚠ Divergences found in OUR code (live gameplay bugs)

1. **`kAmmoMax` (dlls/ts_weapon_info.cpp:112) is the WRONG column.** Its values
   {80,90,90,150,90,170,100,200,90,70,20,40,170} are exactly the **+0x20 range-falloff
   units**, not the reserve cap. Original max carry = **+0xc** (mode 0: 210/175/60/120/70/
   90/160/30/90/150/50/300/35), scaled by `Set_ammo_count(ammocount)`. Effect: our 9mm
   reserve caps at 80 vs original 210, and the ammocount server setting does nothing.
   Consumers: `CTSGun::AddAmmo` (dlls/ts_gun.cpp:371 `TSAmmoMax`), ammo entity spawns.
2. **`kBulletSpeedUnit` (dlls/ts_bulletmgr.cpp:42) is in compile-WRITE order, not caliber
   order.** {330,480,730,280,450,900,790,1000,...} = the order compile_ammo_info happens to
   write entries (1,5,7,2,9,4,6,8,3,10,11,12,13). Correct caliber-ordered speeds:
   {0,330,280,350,900,480,790,730,1000,450,350,400,300,850,0,0}. Effect: slow-mo .45 rounds
   fly at .50AE speed, 5.56 at .45 speed, etc.
3. **No range-damage falloff at all** (ts_bulletmgr.cpp header SLICE note (4) admits it) —
   see compile_range_damage below. Our bullets do flat damage at any distance.
4. `TSRoundsPerMag` (ts_weapon_info.cpp:126) — **verified CORRECT** (+0x10, caliber order).

---

## 1. `compile_ammo_info()` — @0x6f51c (Ghidra 0x7f51c)

SIGNATURE: `void compile_ammo_info(void)`

BEHAVIOR: zeroes all 16×14 dwords of `global_ammo_info`; sets +0x1c = 1.0f for all 16
entries; fills the 13 caliber entries per the table above (prices, kinds, mags, travel
8192, the +0x18 factor, range, speed, name); finally calls `Set_ammo_count(0)` (default
max-carry column). No gmsg/engine calls. It writes entries in the order
1,5,7,2,9,4,6,8,3,10,11,12,13 (source order of the assignments — the trap our
kBulletSpeedUnit fell into).

CALLERS: `compile_weapons_info` @0x7005e (TS_gun.cpp — the one-shot table builder, which
also calls compile_range_damage right after). So the init chain is
compile_weapons_info → { Write_<weapon>×38, compile_ammo_info → Set_ammo_count(0),
compile_range_damage }.

OURS: fragmented: dlls/ts_weapon_info.cpp `kRoundsPerMag` (+0x10, correct),
`kAmmoMax` (mislabeled +0x20), dlls/ts_bulletmgr.cpp `kBulletSpeedUnit` (+0x24, wrong
order). Missing columns: price (+0x4), kind (+0x8), max-carry (+0xc), travel (+0x14),
+0x18, +0x1c, names.

RECONCILE: write a real `s_ammo_info` struct + `global_ammo_info[16]` +
`compile_ammo_info()` in the future dlls/ts_gun.cpp (per the file-consolidation plan;
shared-compile into the client — the client mirrors Set_ammo_count via gmsgTSServerSetting
id 2). Retire kAmmoMax/kRoundsPerMag/kBulletSpeedUnit in favor of `get_ammo_info(cal)->…`.
Behavior-preserving for mags; **playtest-gated** for the reserve-cap change (big buff to
carried ammo, but it's the original behavior). Call it from compile_weapons_info like the
original.

## 2. `get_ammo_info(int)` — @0x6f1c4

SIGNATURE: `s_ammo_info *get_ammo_info(int cal)`

BEHAVIOR: `return (unsigned)cal < 16 ? &global_ammo_info[cal] : &global_ammo_info[0]`.
Entry 0 is the zero entry — out-of-range and caliber-0 (melee) share it.

CALLERS (all real, PLT): CBulletManager::UpdateBullet @0x5ef92 (caches ptr),
ShootBulletsPlayer @0x5f7a9 (caches into static `a_info.1134` @0x1680F8),
CTSGameRules::RestartRound ×6 @0x67c10.. (mag-count clamps vs +0x10),
CBasePlayer::BuyAmmo ×5 @0x84278.. (price check + AddAmmo of +0x10),
CTSGroundWeapon::GetWeapon @0x92498, CTSGroundAmmo::KeyValue @0x9279e (mags×(+0x10)),
CTSSniperAmmo::Spawn ×2 @0xb94cd (+0xc/2), CTSShotgunAmmo::Spawn @0xb9559,
CTSRifleAmmo::Spawn @0xb96d9.

OURS: accessor pair `TSAmmoMax`/`TSRoundsPerMag` (ts_weapon_info.cpp:116/131) — partial.

RECONCILE: implement verbatim next to compile_ammo_info; mechanical rename of call sites
(hldmcompat.cpp:157 spawn-ammo, ts_groundammo.cpp:48, ts_gun.cpp:371, ts_buy.cpp BuyAmmo).
Behavior-preserving except where it fixes divergence (1).

## 3. `Set_ammo_count(int)` — @0x6f2cc

SIGNATURE: `void Set_ammo_count(int mode)`

BEHAVIOR: writes ONLY the +0xc (max carry) column of the 13 calibers per mode —
mode 0/default: 210,175,60,120,70,90,160,30,90,150,50,300,35 (cal 1..13);
mode 1: 90,50,32,60,21,60,40,10,30,34,15,100,15;
mode 2: 30,25,16,30,7,30,20,5,15,17,5,40,5;
mode 3: all 0 (no reserve ammo at all). Any other value = mode-0 table (switch default).

CALLERS: `compile_ammo_info` (mode 0 at startup); `CTSGameRules::Think` @0x67714 — on
`ammocount` cvar change, paired with the gmsgTSServerSetting id-2 broadcast (see
analysis_gmsg_TSServerSetting.md — the client runs its own shared-source copy so its
purchase/ammo bookkeeping matches).

OURS: missing entirely. `ammocount` cvar is registered (dlls/ts_cvars.cpp:32) but dead.

RECONCILE: implement shared (both libs, per the TSServerSetting dossier plan) in
ts_gun.cpp. Server: Think change-detect → Set_ammo_count + MSG_ALL id 2. Client: handler
case 2 calls it. Behavior-preserving (adds a feature that today is inert).

## 4. `compile_range_damage()` — @0x6f1f4  ★ the falloff gap

SIGNATURE: `void compile_range_damage(void)`

BEHAVIOR: fills `range_damage_multipler[0..9999]`:
`m[i] = clamp(1.0f / sqrtf(0.1f * max(i,1)), 0.0f, 1.0f)`.
So m[i] = 1.0 for i ≤ 10, then 1/sqrt(0.1·i): 0.707 @i=20, 0.5 @i=40, 0.316 @i=100,
0.1 @i=1000. Constants: fc_1=1.0, fc_0_1=0.1 (float), clamps vs 1.0/0.0.

CALLERS: `compile_weapons_info` @0x70063 (once at startup).

## 5. `get_range_multipler(int)` — @0x6f290

SIGNATURE: `double get_range_multipler(int idx)` (long double in the gcc2 decompile;
source surely float/double)

BEHAVIOR: clamped LUT read: idx<0 → m[0]; idx≥10000 → m[9999] (DAT_00190dbc =
range_damage_multipler+4·9999); else m[idx].

CALLERS + the INDEX formula (RE'd from disasm, GOT-resolved constants
1/300 @file 0xF6DA0, 10.0 @0xF6DA4):

- **Form A** — `CBulletManager::UpdateBullet` @0x5f1ba, gated on **bulletrec+0x3c == 2**
  (cmpl $0x2,0x3c(%ebp) @0x5f13d): `idx = (int)(dist * (1/300) * 10) = dist/30`,
  then `damage(bulletrec+0x28) *= m[idx]`. Full damage ≤ 300 units; 0.71 @600u;
  0.5 @1200u; 0.32 @3000u. Same form in `ShootBulletsPlayer` @0x5f949 (dist =
  |tr.vecEndPos − src| via the tr.1135 static; also stores dist/300 into static
  `distance.1153` @0x168180; result → `damage_adjust.1154`).
- **Form B** — UpdateBullet @0x5f24a / ShootBulletsPlayer @0x5fdc8 (else-branch):
  `idx = (int)(dist / a_info->rangeUnit[+0x20])` (fpu: st = dist/(10·I) stored to
  distance.1153, ×10 → dist/I). Full damage while dist ≤ 10·rangeUnit (9mm 800u,
  5.56 1500u, 50BMG 2000u, .454 200u), then 1/sqrt(0.1·dist/I).
- **VERIFY**: what bulletrec+0x3c is (most plausibly the ammo `kind` (+0x8) copied into
  the bullet record — kind 2 = rifle/shotgun calibers → form A) and which of SBP's two
  paths carries which gate. Resolve via `re.sh` on UpdateBullet/ShootBulletsPlayer or a
  live `live.py watch` on damage_adjust once the GUI releases the project lock.

OURS: **nothing** — flat damage (confirmed: only radius-damage falloff exists,
dlls/combat.cpp:1038 / ts_grenades.cpp:242; ts_bulletmgr.cpp SLICE note (4)).

RECONCILE (4+5 together): implement LUT + accessor verbatim in ts_gun.cpp, call
compile_range_damage from compile_weapons_info; apply in our bullet-damage path(s)
(FireBulletsPlayer server path + CBulletManager::UpdateBullet) with BOTH forms and the
+0x3c gate once verified. **Playtest-gated** (changes every gunfight at range) and the
single biggest gameplay-parity win in this group.

## 6. `CalcSpread(float &, CBasePlayer *, float)` — @0x7020c

SIGNATURE: `void CalcSpread(float &spread, CBasePlayer *p, float f)`

BEHAVIOR: `mult = |p->pev->velocity| / (100/f)`; clamp to [1.0, 3·f]; `spread *= mult`.
Uses static `spread_mul.1221` @0x16da40 as scratch. I.e. moving faster than 100/f u/s
inflates spread linearly, up to 3·f×: the velocity-accuracy model.

CALLERS: **NONE in the server .so** (only the PIC self-call @0x70210; no @plt entry).
Exported + compiled but dead server-side — apparently superseded by the
ScaleSpread-accumulator system (CTSGun::ScaleSpread @0x702a8 sits immediately after it).
**VERIFY**: whether the CLIENT (shared TS_gun.cpp in client.dll) calls its copy in the
prediction/event path — needs dc.sh/binmap on client.dll (Ghidra locked during this pass)
or a live trace. Until verified, treat as dead code kept for source parity.

OURS: no equivalent; our dynamic accuracy is CTSGun::ScaleSpread (dlls/ts_gun.cpp:854),
which matches the original's live mechanism and has NO velocity term — consistent with
CalcSpread being dead.

RECONCILE: add to ts_gun.cpp for source/audit parity (it's in the binary and named), leave
uncalled like the original. Behavior-preserving (no call sites). Do NOT wire it into fire
paths without the client-side verification.

## 7. `DropKickback(float *, float *, float, float)` — @0x76ea0

SIGNATURE: `void DropKickback(float *kick, float *align, float amt, float slowFactor)`

BEHAVIOR (2-component x/y vectors): if `amt < 0` or `|kick| == 0` → return. Else:
`d = kick − align` (2D), `dist = |d|`, `step = 15 · amt / FixedSlowMul(slowFactor)`
(FixedSlowMul = TS_shared.cpp slow-motion multiplier — slow-mo slows recovery).
`align += d·step` when step ≤ dist else `d·dist` (decompile shows the raw products —
**VERIFY**: likely `(d/dist)`-normalized stepping in source, Ghidra folded the division);
then `kick = normalize(kick) · max(|kick| − step, 0)` — the kick magnitude decays at
15 (deg/units)·amt per call, slow-motion-compensated. Constant dc_15 = 15.0.

CALLERS: **NONE in the server .so** (PIC self-call only). Its producer counterpart
`CTSGun::VKickback` @0x60414 IS live (3 PLT calls inside CTSGun fire paths). Per
analysis_gmsg_TSKickAlign.md the kick-align message was abandoned; DropKickback is the
matching per-frame recoil-recovery — most plausibly consumed CLIENT-side (view punch
decay in the shared source) or dead. **VERIFY** client.dll (same route as CalcSpread).

OURS: client uses stock `V_DropPunchAngle` (cl_dll/view.cpp:1907, `(10 + 0.5·len)/s`),
i.e. HL's decay, not TS's 15/slowMul 2-axis version; server relies on engine punchangle
decay.

RECONCILE: add to ts_gun.cpp uncalled (source parity). If the client-side VERIFY shows
the original client decays its TS kick with it, port into our client view path —
playtest-gated (recoil feel).

## 8. `GetCloseCombatVectors(int, Vector&, Vector&, Vector&, Vector&)` — @0x76e2c

SIGNATURE: mangled `GetCloseCombatVectors__FiR6VectorN31` = (int, Vector&, Vector&,
Vector&, Vector&) — signatures-gcc2.txt's "(int, Vector &, int, int, int)" is the
demangler flattening the N31 repeats.

BEHAVIOR: `out(a2) = src(a1) + 32·forward(a3)`; params a0 (int) and a4 unused in the
compiled body. **32 units** = the melee probe reach baked here.

CALLERS: **NONE in the server .so** (PIC self-call only; it sits right after the
CCombatAttack tail fragment @0x76c8c–0x76e2b). The same `src + 32·v_forward` math appears
inline (written out again in source — gcc 2.96 doesn't auto-inline across functions) in
`KnifeFire` (melee_knifefire.c:196–198, fc_32·v_forward). So the helper is another
compiled-but-unused TS_gun.cpp utility. The melee dossier's live paths route through
CloseCombatAttack/HandleCloseCombat with per-move ranges (48/64/72), not through this.

OURS: no function; our melee ranges live per-move (dlls/ts_gun.cpp:916 knife 48/katana 64,
CCombatAttack move table 64/72). Behaviorally nothing to port.

RECONCILE: add uncalled for parity (trivial). No behavior change.

## 9. `GetBodyByItems(int, int)` — @0x6f0a4

SIGNATURE: `int GetBodyByItems(int weaponType, int items)`

BEHAVIOR: computes the studio **body** value for a weapon's model from its attachment
bits, using five SHORTS in s_weapon_info at `global_weapon_info + type·0x3dc + 0x128..0x130`
(new provenance — these body columns are not in our struct):
- +0x128 silencer body (items bit 1 → body = this)
- +0x12a laser body add (bit 2 → +=)
- +0x12c flashlight body add (bit 4 → +=)
- +0x12e scope body add (bit 8 → +=)
- +0x130 combined laser+flashlight body (used INSTEAD of +0x12a/+0x12c when bits 2|4 both
  set and the field ≠ −1)
Special case: weaponType == 36 (**contender**) without the scope bit → body += 1 (the
model's no-scope submodel). Bits match our attachment mask convention
(ts_weapon_info.h: 1/2/4/8 = silencer/laser/flashlight/scope).

CALLERS: `CTSGun::Deploy` @0x710c4 (sets the deployed model body). Only site.

OURS: missing — dlls/ts_gun.cpp:552 Deploy comment explicitly SLICEs it out ("faithful
Deploy uses the items array + GetBodyByItems"). Our model body never changes, so
silencers/lasers/scopes don't show on the viewmodel.

RECONCILE: add the 5 body shorts to `s_weapon_info` (values recoverable per weapon from
each `Write_<weapon>` — same extraction pattern as scripts/extract_weapons.py) +
implement GetBodyByItems verbatim + set the body in Deploy (and on attachment toggle).
Visual-only, behavior-preserving; verify on-screen (silenced glock body).

## 10. `GetWeaponType(tsweapon_t)` — @0x7705c

SIGNATURE: `char GetWeaponType(tsweapon_t w)` (returns the fire-TYPE byte)

BEHAVIOR: the *category* view of fire-type, with hard special cases BEFORE the table:
- 34 (katana) → 3 (knife); 33 (sawedoff) → 1 (pump/shotgun);
- 16 (uzi_akimbo), 37 (skorpion_akimbo) → 9 (auto/SMG);
- 10 (mk23akimbo), 30 (fnhakimbo), 8 (beretta_akimbo), 21 (gold colts) → 8 (semi/pistol);
- else `get_weapon_info(w)->fireType` (+0x348).
I.e. akimbos count as their underlying pistol/SMG category, katana as a knife, sawed-off
as a shotgun — for restriction/category logic.

CALLERS: `IsRestrictedWeapon` @0x770f3; `CHalfLifeMultiplay::PlayerKilled` @0x79752
(death/drop category logic); `CBasePlayer::DropWeapon` @0x849ad.

OURS: missing; we read `wi->iFireType` raw everywhere (fine for fire dispatch, not for
the category consumers above; TSAkimboBase is a different mapping).

RECONCILE: implement verbatim in ts_gun.cpp; use it in the IsRestrictedWeapon port and
audit PlayerKilled/DropWeapon for where the original consults it. Behavior-preserving
until IsRestrictedWeapon is wired.

## 11. `IsWeaponAkimbo(int)` @0x7019c / 12. `IsOnlyAkimbo(int)` @0x701d4

SIGNATURE: `bool IsWeaponAkimbo(int w)` / `bool IsOnlyAkimbo(int w)`

BEHAVIOR: raw fire-type byte test on `global_weapon_info[w]+0x348`: == 4 (akimbo) /
== 5 (akimbo-only, gold colts). No bounds check (direct `w·0x3dc` index). The CONCAT31
in the decompile is the byte-bool return artifact.

CALLERS: IsWeaponAkimbo — `CTSGun::AddItem` @0x70812, `CTSGun::TakeWeapon` @0x70c25 +
@0x70da4, `CTSGun::GetActiveItems` @0x7125b. IsOnlyAkimbo — `CTSGun::TakeWeapon` @0x70e85,
`CTSGun::GetActiveItems` @0x71270.

OURS: exists-inline — `wi->iFireType == TSFT_AKIMBO` / `== TSFT_AKIMBO_ONLY` at the same
logical sites (dlls/ts_gun.cpp:335,347,388,526,687,1861,1880…). Semantics equivalent.

RECONCILE: mechanical — introduce the two named free functions and swap the inline
comparisons at the sites matching the original's callers (audit parity, zero behavior
change).

## 13. `IsRestrictedWeapon(tsweapon_t, int)` — @0x770c8

SIGNATURE: `bool IsRestrictedWeapon(tsweapon_t w, int allowMask)`

BEHAVIOR: if `(unsigned)(w−1) >= 37` (w outside 1..37, incl. kung-fu 0) → NOT restricted.
If `allowMask == 0` → restricted (everything banned). Else switch on `GetWeaponType(w)`:
- type 1 (pump/shotgun, incl. sawed-off) → restricted unless mask & 0x10 (Shotguns)
- type 3 (knife, incl. katana) → restricted unless mask & 0x1 (Knives)
- type 6,7 (auto rifles/auto shotguns + snipers) → restricted unless mask & 0x8 (Rifles)
- type 8 (semi pistols, incl. pistol akimbos + gold) → restricted unless mask & 0x2 (Pistols)
- type 9 (SMG auto, incl. uzi/skorpion akimbos) → restricted unless mask & 0x4 (SMGs)
- all other types (0, 2 grenade, 4/5 raw akimbo — unreachable post-GetWeaponType, 10 —
  remapped, 11 flag) → never restricted.
Categories line up with the "Kung Knives Pistols SMGs Rifles Shotguns All" strings
(gmsgTSServerSetting dossier); default allow-all mask = 0x1f. Note kung-fu (0) is never
restricted by this function despite the display string.

CALLERS: `CBasePlayer::BuyWeapon` @0x84606 (the buy gate). Mask source = the
`weaponrestriction` value (console cmd `cmd_weaponrestriction`, broadcast as
TSServerSetting id 3; also a CTSMapGlobals field — see the buy dossier).

OURS: missing — dlls/ts_buy.cpp:143 BuyWeapon has an allow-all SLICE stub (comment cites
this exact function @0x770c8).

RECONCILE: implement GetWeaponType + IsRestrictedWeapon in ts_gun.cpp, store the
restriction mask in gamerules (fed by the weaponrestriction command per the
TSServerSetting dossier), gate BuyWeapon on it. Behavior-preserving for stock servers
(default 0x1f = allow all); playtest the restriction path with the console command.

---

## Consolidation note

All 13 belong to the future `dlls/ts_gun.cpp` == original **TS_gun.cpp** (funcfile.tsv),
alongside compile_weapons_info/get_weapon_info (already ours) — the ammo/weapon-table
statics currently in dlls/ts_weapon_info.cpp should fold in per the C1–C19 plan. The
ammo-info + Set_ammo_count pieces are SHARED source (client mirrors them via
gmsgTSServerSetting id 2), so they must compile into both libs like get_weapon_info does.
