# cont.87 — Weapon-select DROP mechanism (both sides), RE'd for a faithful single-function reconstruction

**Goal.** While the weapon-select bar is open, LEFT-click commits (`weapon_<N>`), RIGHT-click DROPS
the highlighted weapon (`drop_<N>`). We currently split the server drop into TWO invented functions
(`CBasePlayer::DropWeapon()` in `ts_gun.cpp` + `CBasePlayer::DropWeapon(int,float)` disarm-only in
`ts_player.cpp`). The original is **ONE** function `CBasePlayer::DropWeapon(tsweapon_t type, float damage)`
@ `nm 0x84900` / Ghidra `0x94900`. This dossier RE's both sides and gives the merged drop-in.

Raw decompiles: `decomp/DropWeapon_server_0x94900.c`, `decomp/CHudTSWeaponBar_Think_client_0x4ce10.c`,
`decomp/button_testclear_0x4fde0.c`.

Enum crib: `0x18`=24=`TSW_M61` (grenade) · `0x1d`=29=`TSW_FLAG` · `0x22`=34=`TSW_KATANA` ·
`0x24`=36=`TSW_CONTENDER` · `0x25`=37=`TSW_SKORPION_AKIMBO` (max, `TSW_COUNT`=38). Weapon id `0x10`=16=
`WEAPON_TS_GUN`. Button bits: `IN_ATTACK`=1, `IN_ATTACK2`=0x800.

---

## Q1 — CLIENT right-click: does it swallow +attack2? close the menu? (ANSWER)

The weapon-bar Think/Draw is `FUN_1004ce10` (client.dll, `this`=CHudTSWeaponBar at `esi`). The relevant
tail (menu-open block gated by `this+0x73c != 0`):

```c
if (*(char *)(this + 0x73c) != '\0') {                 // bar is OPEN and accepting input
    // LEFT-click: IN_ATTACK (mask 1)
    if (ButtonTestClear(&buttons, IN_ATTACK)) {        // FUN_1004fde0(&g_buttons, 1)
        if (col != -1 && (type = slotType(col,row)) >= 0)
            ClientCmd("weapon_%i", type);              // s_weapon__i @ 0x95db8
        *(char *)(this + 0x73c) = 0;                    // CLOSE the menu
    }
    // RIGHT-click: IN_ATTACK2 (mask 0x800)
    if (ButtonTestClear(&buttons, IN_ATTACK2)) {       // FUN_1004fde0(&g_buttons, 0x800)
        if ((type = slotType(col,row)) >= 0)
            ClientCmd("drop_%i", type);                // s_drop__i @ 0x95db0
        *(char *)(this + 0x73c) = 0;                    // CLOSE the menu
    }
}
```

`ButtonTestClear` = `FUN_1004fde0` (`decomp/button_testclear_0x4fde0.c`) is a **test-and-clear on the
global button bitfield** `DAT_100cf970` (the client's live `m_iKeyBits`-equivalent that feeds weapon
prediction/fire):

```c
uint ButtonTestClear(void *unused, uint mask) {
    if ((mask & g_buttons) != 0) { g_buttons &= ~mask; return 1; }   // consume: CLEAR the bit
    return 0;
}
```

**Answers (HIGH confidence — direct decompile):**
- **Yes, it SWALLOWS +attack2**, exactly the same way it swallows +attack for the left-click commit:
  by clearing the button bit out of the live global button field so the click never reaches secondary
  fire. Same helper (`FUN_1004fde0`), same frame.
- **Yes, dropping CLOSES the menu** — `this+0x73c = 0` is set on either click, *unconditionally within
  the click branch* (even if the highlighted slot is empty / `type < 0`: the menu still closes and the
  button is still swallowed; only the `ClientCmd` is skipped).
- **Command string: `"drop_%i"`** formatted with the highlighted slot's `tsweapon_t` (e.g. `drop_5`),
  the exact right-click mirror of the left-click `"weapon_%i"`.
- Order: +attack is tested first, then +attack2, each independent; both branches run in one Think.

**Our client is already faithful here.** `cl_dll/ts_weaponbar.cpp::Think()` already tests+clears
`gHUD.m_iKeyBits & IN_ATTACK` → `Commit()` → `weapon_<N>` and `& IN_ATTACK2` → `drop_<N>`, closing the
menu. One *non-faithful extra*: we keep a persistent `m_swallowAttack/m_swallowAttack2` latch that keeps
re-clearing the bit on later frames until the button is released. The original does **only a single-frame
test-and-clear** (no latch); the held-button-doesn't-fire behaviour comes for free from the deploy's
`m_flNextPrimaryAttack` delay. The latch is harmless (symmetric for both buttons) but is an enhancement,
not a reconstruction — note it, optionally drop it for strict parity. (LOW-impact.)

---

## Q2 — SERVER `CBasePlayer::DropWeapon(tsweapon_t type, float damage)` @0x94900 (full RE)

`void __thiscall CBasePlayer::DropWeapon(CBasePlayer *this, uint type, float damage)` — returns void
(our reconstruction keeps a `bool` for caller convenience; cosmetic). `this_00 = m_pActiveItem` cast to
`CTSGun*`. Field crib (all on the active `CTSGun`): `+0x114`=`m_items[]` (stride `0x38`; `.owned`@+0,
`.clip`@+4, `.rightClip`@+0x20), `+0x658`=grenade count, `+0x65c`=grenade idle-phase (both **placeholder names** — the CTSGun struct
does not yet name these offsets; `m_iGrenades`/`m_iGrenadeIdle` in the code block below are proposed
names to be added/curated). `m_iId`==`0x10` (the code uses `WEAPON_TS_GUN`; add the const if missing —
it's the literal 0x10 per `ts_gun.h:68`). On the player:
`+0x7b4`=`m_flDisarmPressure`, `+0x548`=free-slot counter. `s_weapon_info`: `+0xc8`=`iAkimboType`,
`+0xcc`=`iSingleType`, `+0xd0`=`slotCost` (byte), `+0xdc`=`iStackable`, `+0x348`=`iFireType`.

### Control flow (top to bottom)

1. **Active-gun gate** (HIGH): `pGun = m_pActiveItem`; if null → return. If `pGun->m_iId != WEAPON_TS_GUN`
   (0x10) → return. (The whole arsenal lives on the active CTSGun; even a non-active *type* drops from it.)

2. **Disarm empty-hands guard** (HIGH): `if ((m_weaponType == TSW_FLAG(29) || m_weaponType < 1) && type == -2) return;`
   Can't be disarmed with the flag out or bare hands.

3. **`throw` flag** `local_71` (= `fThrow`/reselect flag passed to `CTSGun::DropWeapon`) defaults `true`.

4. **Disarm pressure gate** (HIGH — only when `type == -2` / `0xfffffffe`):
   ```c
   m_flDisarmPressure += damage;
   int thr = 30;                                   // default (also TSW_KATANA path)
   if (m_weaponType != TSW_KATANA(0x22)) {
       thr = 25;                                   // melee category
       if (GetWeaponType(m_weaponType) != 3) {     // not the melee category
           int sc = pActiveInfo->slotCost;
           thr = (sc < 21) ? 20 : (sc < 50 ? sc : 50);   // clamp(slotCost, 20, 50)
       }
   }
   if (m_flDisarmPressure < (float)thr) return;    // not enough accumulated -> bail
   local_71 = false;                               // disarm => holster (no reselect)
   m_flDisarmPressure = 0;
   ```
   (Matches our `ts_player.cpp` disarm shape; note the threshold branch order — `TSW_KATANA` keeps 30,
   `GetWeaponType==3` keeps 25, else clamp(slotCost,20,50).)

5. **Type resolution** (HIGH): `a0 = type; if ((uint)type > 0x7fffffff) a0 = m_weaponType;` — i.e. any
   **negative** `type` (−1 drop-key, −2 disarm) resolves to the **active** weapon; a **non-negative**
   `type` (from `drop_<N>`) selects **that specific type**. **→ Confirms: dropping a non-active specific
   type is the SAME function with `a0 = type`. There is NO separate "drop non-equipped" method.**

6. **Out-of-range → FLAG drop** (control-flow HIGH; trigger-condition MEDIUM):
   ```c
   if ((uint)(a0 - 1) > 0x24) {          // a0 == 0  OR  a0 > 37  (i.e. not a real weapon slot)
       if (type == -2) return;           // disarm never drops the flag
       if (!HaveFlag(this)) return;
       DropFlag(this, false);            // drop the carried flag
       return;
   }
   ```
   **DIVERGENCE from our code:** the flag is dropped **only** when the resolved type is out of the
   `[1..37]` weapon range — in practice when you're **bare-handed** (`m_weaponType==0`) and press drop.
   Our current `DropWeapon()` calls `DropFlag(false)` **unconditionally at the top**, so we'd drop flag
   **and** weapon on one press. Faithful behaviour drops the flag ONLY in this out-of-range branch.
   (MEDIUM: control flow is unambiguous; the *semantic* "when is a0 out of range" — bare hands only —
   is inferred from the enum range, verify against a live drop of a weapon while flag-carrying.)

7. **Owned gate** (HIGH): `if (!pGun->m_items[a0].owned) return;`

8. **Grenade (a0 == TSW_M61 == 24) special cases:**

   a. **Dead-scatter** (MEDIUM — mode-gated): `if (a0==24 && TSGameRules->[byte<2] && !IsAlive())` →
      if `m_iGrenadeIdle==1` call `GrenadeIdle(1)`, then spawn `m_iGrenades` powerups of type `0x20`
      (`CTSPowerUp::CreatePowerUp(0x20,-1,…)`) at `pev->origin` fanned out (`+5` on x each), `return`.
      (This is the drop-while-dead grenade-scatter branch; gated on some gamerules mode byte `< 2`.)

   b. **Live grenade drop** (HIGH): `if (a0==24)`:
      ```c
      if (m_iGrenadeIdle == 1) GrenadeIdle(1);
      else if (m_iGrenades > 0) {
          TSItem s = m_items[24]; s.clip = 1;          // throw ONE grenade
          pGun = ThrowWeapon(24, s, 0, 0);             // (returns the world entity)
          this+0x548 += wi(24)->slotCost;              // free-slot accounting
          if (m_iGrenades < 2) CTSGun::DropWeapon(24, local_71);   // last one -> remove item + reselect
          else { m_iGrenades--; if (!local_71) HolsterWeapon(); }  // keep, decrement
          goto EMIT_SOUND;                             // weapons/weapondrop.wav
      }
      ```

9. **Non-grenade path** — split by `wi(a0)->iStackable` (`+0xdc`):

   a. **`iStackable == 0` (normal weapon):**
      - **AKIMBO** (HIGH): `if (wi->iFireType == 4 (AKIMBO) && wi->iSingleType != -1)`:
        throw **two** of the `single = iSingleType`, each carrying a **copy of `m_items[single]`** with
        `clip = m_items[akimbo].clip / 2` (both throws get the *same* half — NOT half/total-half), with
        lateral velocity offset **+4** then **−4**; each throw adds `wi(single)->slotCost` to `+0x548`.
        If `type < 0` each thrown gun also gets a pickup expire+owner set (see §10). Then
        `CTSGun::DropWeapon(akimbo, local_71); CTSGun::DropWeapon(single, local_71);` (remove both).
        > Our `ts_gun.cpp` case-1 currently splits `m_iClip` into `half`/`total-half` and copies
        > `m_items[single]`. Faithful is `clip = m_items[akimbo].clip/2` on **both** (differs by ±1 on
        > odd clips) — align it.
      - **SINGLE whose AKIMBO we own** (HIGH): `else if (wi->iAkimboType != -1 && m_items[iAkimboType].owned)`:
        throw ONE of `a0` (copy `m_items[a0]`, `clip = m_items[a0].clip`), then set
        `m_items[a0].clip = m_items[akimbo].rightClip` (the kept barrel), `+=slotCost`,
        `CTSGun::DropWeapon(akimbo, local_71)` (remove the akimbo item, `a0` stays owned),
        `HolsterWeapon()`, then `if (CTSGun::SelectWeapon(a0)) Deploy();` (re-wield the kept single).
        > Our case-2 does the throw + kept-clip but **omits the `HolsterWeapon()`+`SelectWeapon(a0)`+
        > `Deploy()`** re-wield. Add it.
      - **PLAIN** (HIGH): `else { ThrowWeapon(a0, m_items[a0], 0, 0); +=slotCost;
        CTSGun::DropWeapon(a0, local_71); }`.

   b. **`iStackable != 0` (stackable throwable — knife/shuriken, count in `.clip`)** (HIGH):
      ```c
      TSItem s = m_items[a0];
      if (type == -2) s.clip = 1;                  // disarm throws just one unit
      ThrowWeapon(a0, s, 0, 0);
      this+0x548 += wi->slotCost * s.clip;
      if (s.clip < m_items[a0].clip) {             // partial: units remain
          m_items[a0].clip -= s.clip;
          if (!local_71) HolsterWeapon();
      } else {
          CTSGun::DropWeapon(a0, local_71);        // dropped all -> remove item
      }
      ```
      > Our reconstruction has **no** stackable branch. Add it (needs `wi->iStackable`, present in
      > `ts_weapon_info.h` as `iStackable` @+0xdc).

10. **`weapons/weapondrop.wav`** (`EMIT_SOUND_DYN`, CHAN_ITEM, vol 1, attn 0.8, pitch 100) — emitted
    at `EMIT_SOUND` for the grenade/normal/stackable paths (not the dead-scatter/flag early returns).

11. **Pickup expire + owner on the thrown gun** (HIGH — the key `type<0` vs `>=0` behavioural split):
    ```c
    if (type >= 0 || a0 == TSW_FLAG(0x1d) || lastThrownGun == NULL) goto RESPEED;
    lastThrownGun->pev->[+0xc4] = clamp(gpGlobals->time + 2*weaponstay, +10..+120 window);
    lastThrownGun->pev->[+0xd0] = this->edict();   // owner (no instant self-repickup)
    ```
    **A weapon dropped by the drop KEY / disarm (`type<0`) gets a pickup delay + owner; a weapon dropped
    from the select-bar RIGHT-CLICK (`drop_<N>`, `type>=0`) does NOT** — it's immediately re-pickable by
    anyone. (Same expire math is applied per-throw inside the akimbo branch for `type<0`.) The clamp is
    `if (2*weaponstay >= 120) t=time+120; else if (2*weaponstay <= 10) t=time+10; else t=time+2*weaponstay`
    (the `weaponstay` cvar `+0xc`; `.rodata` consts 120/10). MEDIUM on the exact field offsets
    (`pev+0xc4`/`+0xd0` are on the CTSWorldGun, not decoded to names here — likely `m_flPickupTime`/owner).

12. **Speed recompute** (HIGH): `RESPEED:` → `SetPlayerSpeed(GetSpeedBySlots(this));` — dropping frees
    slot weight → recompute movement speed. (Our `+0x548` counter is *derived* from the arsenal in our
    build — see `ts_player.cpp:678` — so the explicit `+= slotCost` writes are not needed; the final
    `SetPlayerSpeed(GetSpeedBySlots(this))` recomputes from the now-shrunk arsenal. Keep the final call.)

### Callers / param convention (confirmed via `objdump` on the `@plt` call sites, cdecl):
`DropWeapon(this, type, damage)`, `damage` pushed first. Death/disconnect loops call
`DropWeapon(this, /*type*/1..N, 0.0)` per owned type (positive → no expire/owner, matches §11).

- **drop KEY** (`drop`, exact): `player->DropWeapon(-1, 0.0f)` — active weapon, `type<0` → expire+owner set.
- **`drop_<N>`** (strncmp `"drop_"`, 5): `player->DropWeapon(atoi(cmd+5), 0.0f)` — specific type `N`,
  `type>=0` → no expire/owner, reselect if it was active.
- **disarm** (melee hit): `player->DropWeapon(-2, flDamage)` — accumulate pressure, gate on threshold.

Both `"drop_"` (`.rodata 0xfb6b1`) and `"drop"` (`0xfb6b7`) exist in `ts_i386.so`, and `"weapon_"`
(`0xf8804`) — so the original server ClientCommand has a `strncmp("drop_",5)` handler alongside the exact
`"drop"` and the `strncmp("weapon_",7)`. **Our `dlls/client.cpp` currently has `"drop"` but NO `"drop_"`
handler** — add it (§3).

---

## 3 — Server `drop_<N>` ClientCommand handler (mirror of `weapon_<N>`)

`dlls/client.cpp`, right after the `"drop"` exact handler (~line 641) / mirroring the `"weapon_"` block
(~596):

```cpp
else if (!strncmp(pcmd, "drop_", 5) && pcmd[5] >= '0' && pcmd[5] <= '9')
{
    // TS weapon-select RIGHT-click: the select bar emits "drop_<N>" (numeric tsweapon_t) to drop
    // that specific owned type from the arsenal (reference/dossiers/cont87/weapon_drop.md). type>=0
    // => no pickup-owner/expire (unlike the drop KEY), reselect if it was active. The digit guard
    // avoids the exact "drop" key command below.
    player->DropWeapon(atoi(pcmd + 5), 0.0f);
}
```

and the drop KEY handler becomes (unify onto the single signature):

```cpp
else if (FStrEq(pcmd, "drop"))
{
    player->DropWeapon(-1, 0.0f);   // active weapon; type<0 => pickup delay + owner set
}
```

---

## 2 — The merged drop-in: single `bool CBasePlayer::DropWeapon(int type, float flDamage)`

Replaces BOTH `ts_gun.cpp::DropWeapon()` and `ts_player.cpp::DropWeapon(int,float)`. Put it in
`ts_gun.cpp` (server-only block, next to `TSThrowWorldGun`/`ThrowWeapon`). Declared once in `player.h`
(drop the no-arg overload). Uses our helpers: `GetPlayerTSGun`, `get_weapon_info`, `TSThrowWorldGun`,
`CTSGun::DropWeapon`, `HolsterWeapon`, `SelectWeapon`, `GrenadeIdle`, `HaveFlag`/`DropFlag`,
`GetSpeedBySlots`/`SetPlayerSpeed`, `TSItem m_items[]`.

```cpp
// @0x84900 CBasePlayer::DropWeapon(tsweapon_t type, float damage) — the ONE drop entry point.
//   type <  0  -> the ACTIVE weapon   (-1 = drop key; -2 = disarm, with the pressure gate)
//   type >= 0  -> that SPECIFIC owned type (from the weapon-select "drop_<N>")
// Faithful merge of our previously-split DropWeapon()/DropWeapon(int,float). See
// reference/dossiers/cont87/weapon_drop.md.
bool CBasePlayer::DropWeapon(int type, float flDamage)
{
	CTSGun *pGun = GetPlayerTSGun(this);
	if (!pGun || pGun->m_iId != WEAPON_TS_GUN)   // whole arsenal lives on the active TS gun
		return false;

	// (2) disarm can't take the flag or bare hands
	if ((pGun->m_weaponType == TSW_FLAG || pGun->m_weaponType < 1) && type == -2)
		return false;

	bool fThrow = true;   // true => reselect after (CTSGun::DropWeapon); false (disarm) => holster only

	// (4) disarm pressure gate
	if (type == -2)
	{
		m_flDisarmPressure += flDamage;
		int thr = 30;                                     // katana / default
		if (pGun->m_weaponType != TSW_KATANA)
		{
			thr = 25;                                     // melee category
			if (GetWeaponType((tsweapon_t)pGun->m_weaponType) != 3)
			{
				int sc = get_weapon_info((tsweapon_t)pGun->m_weaponType)->slotCost;
				thr = (sc < 21) ? 20 : (sc < 50 ? sc : 50);
			}
		}
		if (m_flDisarmPressure < (float)thr)
			return false;
		fThrow = false;
		m_flDisarmPressure = 0.0f;
	}

	// (5) resolve type: negative -> active; non-negative -> that specific type
	const int a0 = (type < 0) ? pGun->m_weaponType : type;

	// (6) out of weapon range -> flag drop (bare-handed drop key)
	if (a0 < 1 || a0 >= TSW_COUNT)   // a0 == 0 (bare hands) or > 37
	{
		if (type == -2)
			return false;
		if (!HaveFlag(this))
			return false;
		DropFlag(false);
		return true;
	}

	// (7) must own it
	if (!pGun->m_items[a0].owned)
		return false;

	const s_weapon_info *wi = get_weapon_info((tsweapon_t)a0);

	// (8b) grenade
	if (a0 == TSW_M61)
	{
		// (8a) dead-scatter branch omitted here: gate on !IsAlive() + gamerules mode; scatter
		// m_iGrenades powerups of type 0x20. VERIFY the gamerules-mode byte before wiring.
		if (pGun->m_iGrenadeIdle == 1)     // cooking -> just abort the cook
			pGun->GrenadeIdle(1);
		else if (pGun->m_iGrenades > 0)
		{
			TSThrowWorldGun(this, pGun, TSW_M61, 1, 0.0f);   // throw exactly one
			// (11) type<0 => pickup delay+owner on the thrown grenade (see dossier §11)
			if (pGun->m_iGrenades < 2)
				pGun->DropWeapon(TSW_M61, fThrow);            // last one -> remove + reselect
			else
			{
				pGun->m_iGrenades--;
				if (!fThrow)
					pGun->HolsterWeapon();
			}
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
			SetPlayerSpeed(GetSpeedBySlots(this));
			return true;
		}
		SetPlayerSpeed(GetSpeedBySlots(this));
		return false;
	}

	// (9b) stackable throwable (knife/shuriken): .clip is a unit count
	if (wi->iStackable)
	{
		int throwN = (type == -2) ? 1 : pGun->m_items[a0].clip;   // disarm throws one
		TSThrowWorldGun(this, pGun, a0, throwN, 0.0f);
		if (throwN < pGun->m_items[a0].clip)
		{
			pGun->m_items[a0].clip -= throwN;
			if (!fThrow)
				pGun->HolsterWeapon();
		}
		else
			pGun->DropWeapon(a0, fThrow);
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
		SetPlayerSpeed(GetSpeedBySlots(this));
		return true;
	}

	// (9a) normal weapon
	if (wi->iFireType == TSFT_AKIMBO && wi->iSingleType >= 1 && wi->iSingleType < TSW_COUNT)
	{
		// combinable akimbo -> throw TWO of the base single, each with akimbo.clip/2, ±4 lateral
		const int single = wi->iSingleType;
		const int half = pGun->m_items[a0].clip / 2;
		TSThrowWorldGun(this, pGun, single, half, 4.0f);
		TSThrowWorldGun(this, pGun, single, half, -4.0f);
		pGun->DropWeapon(a0, fThrow);        // remove akimbo (auto-selects next)
		pGun->DropWeapon(single, fThrow);    // and the single base if also owned
	}
	else if (wi->iAkimboType >= 1 && wi->iAkimboType < TSW_COUNT && pGun->m_items[wi->iAkimboType].owned)
	{
		// single whose akimbo we ALSO own -> throw one, keep the other barrel, re-wield the single
		const int akimbo = wi->iAkimboType;
		const int keptClip = pGun->m_items[akimbo].rightClip;
		TSThrowWorldGun(this, pGun, a0, pGun->m_items[a0].clip, 0.0f);
		pGun->m_items[a0].clip = keptClip;
		pGun->DropWeapon(akimbo, fThrow);    // remove the akimbo item; a0 stays owned
		pGun->HolsterWeapon();
		pGun->SelectWeapon(a0);              // re-wield the kept single (orig: if(SelectWeapon)Deploy)
	}
	else
	{
		// plain weapon
		TSThrowWorldGun(this, pGun, a0, pGun->m_items[a0].clip, 0.0f);
		pGun->DropWeapon(a0, fThrow);
	}

	// (11) type<0 (drop key / disarm) => set pickup delay + owner on the last thrown gun; type>=0
	// (drop_<N>) leaves it immediately re-pickable. VERIFY the CTSWorldGun field names (pev+0xc4/+0xd0).

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/weapondrop.wav", VOL_NORM, ATTN_NORM);
	SetPlayerSpeed(GetSpeedBySlots(this));   // (12) dropping freed slot weight -> recompute speed
	return true;
}
```

### How each caller invokes the single function
| Caller | Call | `type` path | expire/owner (§11) |
|---|---|---|---|
| drop KEY (`"drop"`, default.cfg binds `t`) | `DropWeapon(-1, 0.0f)` | active weapon | **yes** |
| select-bar RIGHT-click (`"drop_<N>"`) | `DropWeapon(atoi(N), 0.0f)` | specific type N | **no** |
| melee disarm (CloseCombatAttack, `bDisarm`) | `DropWeapon(-2, flDamage)` | active + pressure gate | yes |
| death/disconnect scatter | `DropWeapon(t, 0.0f)` per owned `t` | specific | no |

---

## Confidence & flags

- **HIGH** — Q1 client swallow (both buttons test-and-clear the same helper), menu close, `drop_%i`
  string; the single-function type resolution (neg→active, ≥0→specific), owned gate, disarm pressure
  gate + thresholds, grenade throw-one/decrement, akimbo two-single throw (±4), single-with-akimbo
  kept-barrel, stackable count branch, the final `EMIT_SOUND`+`SetPlayerSpeed`, and the caller param
  convention (cdecl `(this,type,damage)`, `damage` first, positive types with `0.0`).
- **HIGH structurally / MEDIUM semantically** — §6 FLAG drop lives ONLY in the out-of-range branch
  (fires when `a0∈{0,>37}`, i.e. bare-handed). This is a real DIVERGENCE: our current code drops the
  flag unconditionally at the top of `DropWeapon()`. Verify with a live drop while carrying the flag
  AND a weapon (does the flag stay?).
- **MEDIUM** — §8a dead grenade-scatter (gamerules mode byte `<2`; powerup type 0x20 fan-out) — left as
  a TODO in the merged code, wire only after confirming the gamerules gate. §11 pickup expire+owner
  field offsets (`CTSWorldGun pev+0xc4`/`+0xd0`) not decoded to names; the `type<0` vs `>=0` split
  itself is HIGH, the exact fields are MEDIUM.
- **LOW-impact fidelity notes** — akimbo clip split is `akimbo.clip/2` on BOTH throws (ours does
  `half`/`total-half`, off-by-one on odd clips); single-with-akimbo needs the added
  `HolsterWeapon()`+`SelectWeapon(a0)` re-wield; our client's persistent `m_swallowAttack*` latch is an
  enhancement over the original's single-frame test-and-clear.
- **`local_71` (the throw-vs-holster flag)** = `fThrow` passed to `CTSGun::DropWeapon(type, fThrow)`:
  default `true` (normal drop → reselect via `GetBetterWeapon`), set `false` on a **successful disarm**
  (→ `SyncActiveToItem` holster, no reselect) and used as `if(!fThrow) HolsterWeapon()` in the
  keep-item (grenade-decrement / stackable-partial) branches. HIGH confidence.
