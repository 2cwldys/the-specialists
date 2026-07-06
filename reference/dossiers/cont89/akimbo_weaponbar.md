# cont.89 — Akimbo weapon-bar clip + deploy-flicker (original client → OUR server)

**Symptom cluster (original TS `hl.exe`/`client.dll` vs OUR `hl.so`, akimbo uzis):**
1. **Swap flicker.** Switching to akimbo uzis shows a SINGLE-uzi viewmodel for ~1 s,
   then snaps to the akimbo view/body — the akimbo body isn't set on the deploy frame.
2. **Weapon-bar phantom clip.** With akimbo uzis the bar shows akimbo = **64/64**
   (correct) but the base single uzi ALSO lingers, drawn **red** with a bad clip
   ("-1/32").
3. **VERIFY items:** `vuser4.z`/`item+0x0c` ("second/left clip", LOW), and
   `item+0x08/0x0c/0x1c` = bloody / second-clip / leftClip.

All originals decompiled from `reference/ts_i386.so` (server, symboled; Ghidra addr =
nm + 0x10000) and `reference/client.dll` (weapon bar; PE imagebase `0x10000000+RVA`).
`pm_shared`/`TS_gun.cpp` are one source compiled into `.so`+`mp.dll`+`client.dll`, so
the `.so`/`client.dll` bodies are exactly what the original predicts/sends.

---

## 0. TL;DR — root causes

| # | Bug | Root cause (RE-confirmed) | Fix side |
|---|---|---|---|
| 1 | Combine single-clip | **NOT a combine bug.** Original PRESERVES the single's clip through the combine (never zeroed/sentineled); single stays owned. Our `TakeWeapon` matches. | — (already correct) |
| 2 | Phantom red single in bar | Original re-sends `WStatus` per slot **only on the OWNED byte changing** + a **targeted paired-variant resend** (akimbo↔single) on **active-weapon / reserve change**. OUR `WStatus` loop is (a) gated *behind* the `WeaponInfo` change early-return and (b) change-detected on the **clip**, and we **omit the paired-variant resend** — so the base single's bar cell is left STALE (last-fired / uninitialized) and renders red. | **server** (`ts_gun.cpp` UpdateClientData) |
| 3 | Deploy flicker | Original client's **draw.sc event** (`FUN_100425b0`) sets the viewmodel deploy anim **+ body = GetBodyByItems(type, attach)** on the deploy frame. OUR `EV_TSDraw` only updates the predicted-state cache — it never sets the viewmodel body, so the body is a frame(s) late until prediction catches up. | **client** (`ts_weapon_predict.cpp` EV_TSDraw) |
| 4 | vuser4.z / second clip | The akimbo **LEFT clip rides `clientdata+0x70` (`ammo_nails`)** (= `item+0x1c`); the client derives **rightClip = clip − leftClip**. `vuser4.z` (cd+0x1d8) carries a **different** field `item+0x0c`. OUR `cd->vuser4.z = it.leftClip` is wrong on BOTH counts (leftClip belongs on cd+0x70; vuser4.z should be item+0x0c). | **server + client** |

The single lingering in the bar is **faithful** — the original bar ALSO draws the
owned single beside the akimbo (separate grid cells). Only the CLIP shown is wrong,
because our server never refreshes the single's bar cell the way the original does.

---

## 1. Combine single-clip — original PRESERVES it (both match)

`CTSGun::TakeWeapon` — server nm `0x70a04` (Ghidra `0x80a04`). Slot layout (from the
decompile; slot base = `field_0x114 + type*0x38`, int-stride `0xe`):

| item off | field | | item off | field |
|---|---|---|---|---|
| +0x00 | owned (byte) | | +0x14 | active mask |
| +0x02 | fireMode (byte) | | +0x1c | **leftClip** (int) |
| +0x04 | **clip** (int) | | +0x20 | **rightClip** (int) |
| +0x08 | bloody | | +0x24 | altMode (byte) |
| +0x0c | *second-clip* (int, see §4) | | +0x28 | anim-block ptr |
| +0x10 | attach mask | | +0x2c | holdPosition |

The combine (fresh-fill, `a1==0`, `IsWeaponAkimbo(fill)` branch):
```c
akimbo.leftClip  = single.clip;                 // item+0x1c = single's CURRENT clip
akimbo.rightClip = weapon_info[single].iClip;   // item+0x20 = a fresh full clip
akimbo.clip      = leftClip + rightClip;         // item+0x04 total
// UNION attach (item+0x10) and active (item+0x14) across akimbo & single
//   -> single.attach/active are updated; single.clip is NOT touched, single.owned stays 1
```
**The single item's clip is never modified in the combine** — it keeps whatever it
had, and the single stays owned. OUR `TakeWeapon` (`dlls/ts_gun.cpp:499-514`) is a
faithful match (`it.leftClip = m_items[single].owned ? m_items[single].clip : singleClip;`
`it.rightClip = singleClip;` single kept). ✅ **No fix.**

Where the single's clip *does* track after the combine (both original & ours):
- **AkimboFire** mirrors `single.clip = akimbo.leftClip` each shot (orig `@0x82c3a`;
  ours `ts_gun.cpp:2293`).
- **Reload completion** sets `leftClip = clip/2`, `rightClip = clip − leftClip`, and
  mirrors `single.clip = leftClip` (orig `CBasePlayerWeapon::ItemPostFrame @0xa0455`;
  ours `dlls/weapons_shared.cpp:189-195`). ✅

So on a faithful server the single's clip is always a real count (32 → 0 as you drain
the LEFT barrel → 32 on reload). A literal "-1" is therefore **not** a combine
artifact — it is the bar CELL going stale (§2).

---

## 2. Weapon-bar clip channel — the `WStatus` divergence (the phantom red single)

### 2a. Wire format (identical) + change-detection (DIVERGENT)

Server writer = `CTSGun::UpdateClientData` — nm `0x758c8` (Ghidra `0x858c8`). The
whole body is throttled behind `field_0x9e0` (`gpGlobals->time + 0.1`). Inside, TWO
distinct `WStatus` mechanisms:

**(A) The per-slot owned loop — UNCONDITIONAL each throttle tick.** Walks all 38 slots
(type 0x25→0), change-detecting **only on the OWNED byte** (`slot+0x00`) against a
per-slot shadow cache; on change:
```
WStatus = { BYTE type, BYTE owned(slot+0x00), BYTE clip(slot+0x04) }
```
So a slot's bar cell is (re)sent **only when its ownership flips** (0→1 pickup /
1→0 drop). The clip byte is a snapshot **at the ownership change**.

**(B) The targeted paired-variant resend — after the `WeaponInfo` send, gated on the
active weapon's `m_weaponType`/reserve changing:**
```c
int ak = get_weapon_info(m_weaponType)->iAkimboType;   // weapon_info+0xc8
if (ak != -1 && m_items[ak].owned)
    WStatus{ ak, m_items[ak].owned, m_items[ak].clip };
int sg = get_weapon_info(m_weaponType)->iSingleType;   // weapon_info+0xcc
if (sg != -1 && sg != m_weaponType && m_items[sg].owned)
    WStatus{ sg, m_items[sg].owned, m_items[sg].clip };
```
This is what keeps the base single's bar clip **fresh**: every time you switch to the
akimbo (or its reserve changes on reload), the original re-sends the single's `WStatus`
with the single's live `slot+0x04` (= the mirrored `leftClip`). Result: the single
always shows a real count.

**OUR version (`dlls/ts_gun.cpp:2459-2508`):**
- The whole `WStatus` loop is placed **after** the `WeaponInfo` change early-return
  (`return false;` at 2460-2463) — so it runs ONLY when the active weapon's
  `WeaponInfo` changed, not unconditionally.
- The loop change-detects on the **clip** (`state = owned ? clip : -1`) rather than on
  OWNED.
- **We omit mechanism (B) entirely.**

Consequence: the base single's bar cell is refreshed only incidentally (whenever the
active weapon's WeaponInfo happens to change AND our clip-diff for the single fires).
After the combine/switch/reload the single's cell is left at a stale value (its
last-fired `leftClip`, or the client-side init `-1` if a fresh combine never triggered
a resend) → the original client draws it red / with a garbage count.

### 2b. Client draw — cell drawn if type != -1; RED when clip < 1 (both cells shown)

Original bar draw `FUN_1004e030` (RVA 0x4e030) + WStatus handler `FUN_1004f990`
(RVA 0x4f990, via `FUN_1004c840`). The bar is a `(col,row)` grid (col = weapon_info
`+0x85`, row = weapon_info `+0x21`); each cell stores **type @ +0x164** and
**clip @ +0x168**. Handler:
```c
type  = READ_BYTE(); owned = READ_BYTE();
if (weapon_info+0x85 < 0x11 && weapon_info+0x21 < 0xb) {
    grid.type[col,row] = (owned == 0) ? -1 : type;   // -1 == empty cell
    grid.clip[col,row] = READ_BYTE();                // 3rd byte (unsigned 0..255)
}
```
Draw: a cell is drawn iff `grid.type != -1`; it is tinted **RED (0xc3,0x2d,0x2d)**
when `grid.clip < 1` (i.e. == 0, since the byte is unsigned). The single and the
akimbo occupy DIFFERENT grid cells (different weapon_info row), so **both draw** —
the original shows the single beside the akimbo. The mouse-scroll cursor
(`FUN_1004f500`) SKIPS an akimbo-paired single, but the DRAW does not. Our client
(`cl_dll/ts_weaponbar.cpp` `MsgFunc_WStatus`/`Draw`) matches this (per-slot `m_iClip`,
skip `< 0`, red at 0) — no client-side draw fix needed beyond the note below.

> Note: the original reds any owned cell whose clip `< 1`; ours reds only `== 0`
> (`ts_weaponbar.cpp:388`). Equivalent for a non-negative byte; leave as-is.

### 2c. Fix (server-only) — become drop-in with the original bar

In `CTSGun::UpdateClientData` (`dlls/ts_gun.cpp:2426`):
1. **Move the per-slot `WStatus` owned-loop ABOVE the `WeaponInfo` early-return**
   (2460-2463) so it runs every call (like the original's mechanism A, throttle
   permitting), and change-detect it on the **owned flag** (send `{type, owned,
   type==active ? m_iClip : m_items[type].clip}`), not the clip.
2. **Add mechanism (B) after the `WeaponInfo` `MESSAGE_END()` (2477):** when
   `m_weaponType`/reserve changed, resend `WStatus` for the active weapon's
   `iAkimboType` (if owned) and `iSingleType` (if owned && != active). This refreshes
   the base single's bar cell to its true mirrored clip the instant you switch to the
   akimbo — killing the stale/red phantom. Sketch:
   ```cpp
   auto sendW = [&](int t){
       const int c = (t == m_weaponType) ? m_iClip : m_items[t].clip;
       m_iClientWStatus[t] = c;                    // keep the cache coherent
       MESSAGE_BEGIN(MSG_ONE, gmsgTSWeaponStatus, NULL, pPlayer->pev);
       WRITE_BYTE(t); WRITE_BYTE(1); WRITE_BYTE(c); MESSAGE_END();
   };
   const s_weapon_info *awi = get_weapon_info((tsweapon_t)m_weaponType);
   if (awi->iAkimboType >= 0 && m_items[awi->iAkimboType].owned) sendW(awi->iAkimboType);
   if (awi->iSingleType >= 0 && awi->iSingleType != m_weaponType &&
       m_items[awi->iSingleType].owned) sendW(awi->iSingleType);
   ```
   (Client already reads it correctly — `ts_weaponbar.cpp:190-203`.)

**Net:** the phantom single now shows its real count (32 → 0 red as the left barrel
drains → 32 on reload), matching the original, on BOTH our client and the original
client.

---

## 3. Deploy flicker — the body is set client-side by the draw.sc EVENT, and ours doesn't

### Server Deploy is correct
`CTSGun::Deploy` (nm `0x71030`, Ghidra `0x81030`): sets `m_pActiveItem/Info`, computes
`body = GetBodyByItems(m_weaponType, attachEquipped)`, stores it into `pev->body` AND
passes it to `DefaultDeploy(viewModel, playerModel, drawSeq, animExt, 0, pev->body)`,
then fires the **draw.sc event** (`m_usDraw`, iparam1=type, iparam2=attach) and
`CTSAnimation::Animate(player, 0x34)`. The akimbo uzi has its **own** viewmodel
(`models/v_uzi_akimbo.mdl`, `ts_weapon_info.cpp:320`) — it is NOT a body submodel of
`v_uzi.mdl`; `GetBodyByItems` (nm `0x6f0a4`) only encodes **attachments** (sil/laser/
flash/scope), returning 0 for a plain akimbo. So `pev->viewmodel`/`pev->body` are set
correctly and immediately on the server. Our `Deploy` (`ts_gun.cpp:771`) matches.

### The client sets the deploy body IN the draw event — ours is a no-op
Original draw.sc handler `FUN_100425b0` (RVA 0x425b0), **local player only**:
```c
type   = args->iparam1;                         // param+0x38
attach = args->iparam2;                         // param+0x3c
if (is_local(args->entindex)) {
    info = get_weapon_info(type);
    body = GetBodyByItems(type, attach);        // FUN_10007110 == GetBodyByItems
    // engine viewmodel call: (drawSeq = *(short*)(animblock+4), body)
    studio_SetWeaponAnimBody( info->drawSeq, body );   // DAT_100d5c78+0x40
}
```
i.e. on the deploy frame the client itself plays the deploy anim WITH the computed
body — it does NOT wait for the next prediction/clientdata to establish the body.

**OUR `EV_TSDraw` (`cl_dll/ts_weapon_predict.cpp:39`)** is:
```cpp
void EV_TSDraw(event_args_t* args) { g_tsActive.UpdateFromEvent(args); }
```
It updates the predicted-state cache but **never sets the viewmodel anim/body**. So the
body/anim for the newly-deployed weapon is established only later, when the vuser
reconcile (`vuser3.x = m_weaponType`, `dlls/client.cpp:2190`) lands and
`TS_PredictedViewmodelBody`/`HUD_SendWeaponAnim` (`cl_dll/hl/hl_weapons.cpp:62,785`)
next run — a frame(s)-late body that reads as the "single-then-akimbo" snap. (The
viewmodel MODEL swap is server-authoritative via `from->client.viewmodel`,
`hl_weapons.cpp:661`; the visible lag is the deploy body/anim not being asserted on the
deploy frame — exactly the parent's "akimbo body isn't predicted immediately.")

### Fix (client-only) — mirror `FUN_100425b0` in `EV_TSDraw`
In `cl_dll/ts_weapon_predict.cpp:39`, after `UpdateFromEvent`, for the **local**
player (`EV_IsLocal(args->entindex)`) set the viewmodel deploy anim + body immediately:
```cpp
void EV_TSDraw(event_args_t* args) {
    g_tsActive.UpdateFromEvent(args);
    if (!EV_IsLocal(args->entindex)) return;
    const int type   = args->iparam1;
    const int attach = args->iparam2;
    const s_weapon_info* wi = get_weapon_info((tsweapon_t)type);
    const int body   = GetBodyByItems(type, attach);   // client copy already exists
    const int drawSeq = /* wi draw sequence, s_drawSeq[type] */;
    gEngfuncs.pEventAPI->EV_WeaponAnimation(drawSeq, body);  // set anim+body on the deploy frame
}
```
(`GetBodyByItems` is already available client-side — `hl_weapons.cpp:67` uses it. Use
the same `s_drawSeq[]`/anim-block draw index the deploy uses.) This asserts the akimbo
body/anim on the deploy frame, ending the ~1 s single-look. Server side is already
correct — no server change for the flicker.

> ⚠ Confidence: HIGH on the RE (the original draw handler demonstrably sets the body;
> ours doesn't). The exact ~1 s duration should be confirmed with a live gdb watch on
> the deploy — the mechanism (deploy-frame body assertion) is the fix regardless.

---

## 4. `vuser4.z` / second-clip — the akimbo prediction channel (both-sides correction)

DLL `UpdateClientData` (the clientdata weapon block) — nm `0xa0294` (Ghidra `0xb0294`),
runs when `sendweapons`. `iVar4` = gun, `iVar4+0xcc` = `m_pActiveItem`. What it packs:

| clientdata off | field name | source (`m_pActiveItem + …`) | our name |
|---|---|---|---|
| **+0x70** | **ammo_nails** | **item+0x1c = leftClip** | akimbo LEFT barrel |
| +0x68 | m_iId | 0x10 | weapon id |
| +0x6c | ammo_shells | queued weaponanim | pending anim |
| +0x1c4 | (vuser3.x) | item — `(int)(gun+0xc8)` m_weaponType | active type |
| +0x1c8 | (vuser3.y) | item+0x02 mode | fireMode |
| +0x1cc | (vuser3.z) | item+0x10 attach | equipped mask |
| +0x1d0 | (vuser4.x) | item+0x24 altMode | altMode |
| +0x1d4 | (vuser4.y) | item+0x04 clip | active clip (total) |
| **+0x1d8** | **(vuser4.z)** | **item+0x0c** (NOT leftClip) | *second-clip* |
| +0x1b8 | (vuser2.x) | item+0x08 bloody | bloody |
| +0x1c0 | (vuser2.z) | item+0x2c holdPosition | holdPosition |
| +0x1b4 | (vuser1.z) | item+0x14 active | active mask |

Client read-back `HUD_WeaponsPostThink` (`FUN_10003830`, the m_iId==0x10 branch)
reconstructs the akimbo barrels as:
```c
status[+0x04] = clip      (from cd+0x1d4)
status[+0x1c] = leftClip  = cd+0x70            // ammo_nails
status[+0x20] = rightClip = clip - leftClip, clamped >= 0
status[+0x0c] = cd+0x1d8                        // the vuser4.z "second-clip"
```
So the client **derives rightClip = clip − leftClip**; the authoritative barrel input
is `clip` (cd+0x1d4) and `leftClip` (**cd+0x70 = `ammo_nails`**). `vuser4.z`
(cd+0x1d8) is a **separate** field (`item+0x0c`), stored at `status+0x0c`.

### Our divergence
`dlls/client.cpp:2195-2197`:
```cpp
cd->vuser4.y = (float)tsgun->m_iClip;     // cd+0x1d4 = clip  ✅
cd->vuser4.z = (float)it.leftClip;         // cd+0x1d8  ✗  (this is item+0x0c in the original)
```
We put `leftClip` on `vuser4.z`, and we **never write cd+0x70 (`ammo_nails`)** with
`leftClip` — instead `ammo_nails` carries `ammo_bolts` (0 for TS)
(`hl_weapons.cpp:756`). So against the ORIGINAL client the akimbo left barrel arrives
as **0** → rightClip = clip → both barrels predicted from one side (barrel desync /
wrong dry-click side). Our OWN client compensates because our `hl_weapons.cpp:626`
reads `vuser4.z` back into `it.leftClip` — self-consistent, but NOT drop-in.

### Fix (server + client)
- **Server** (`dlls/client.cpp`): send the akimbo LEFT clip on `cd->ammo_nails`
  (= item+0x1c leftClip), and set `cd->vuser4.z = (float)it.<item+0x0c>` (see below).
  ```cpp
  cd->ammo_nails = it.leftClip;      // cd+0x70 — the original's akimbo left-barrel channel
  cd->vuser4.z   = 0.0f;             // cd+0x1d8 = item+0x0c (see semantic note)
  ```
- **Client** (`cl_dll/hl/hl_weapons.cpp`): read leftClip from `from->client.ammo_nails`
  and derive rightClip:
  ```cpp
  it.leftClip  = (int)from->client.ammo_nails;                       // was vuser4.z
  it.rightClip = it.clip - it.leftClip; if (it.rightClip < 0) it.rightClip = 0;
  ```
  and on the pack side `to->client.ammo_nails = it.leftClip;` (replacing the
  `ammo_bolts` use for the tsgun — TS has no nails/bolts weapon). Remove the
  `it.leftClip = vuser4.z` read at `hl_weapons.cpp:626`.

### `item+0x0c` semantic — VERIFY (LOW)
`item+0x0c` is zeroed on fresh fill (`TakeWeapon`) and rides `vuser4.z`/`status+0x0c`;
it is distinct from clip(0x04)/leftClip(0x1c)/rightClip(0x20). Its writer was not
pinned in this pass (it is NOT the akimbo barrel — that's cd+0x70). Sending 0 for it is
safe and matches a fresh item. **Recommend a 30-second live confirm** (the original
`hl.exe` is available: `live.py break mp.dll <UpdateClientData>` and read
`m_pActiveItem+0x0c` while akimbo-firing/reloading) before assigning it a name — do not
invent one. Until then: `cd->vuser4.z = 0` is the honest drop-in value.

---

## 5. Provenance (addresses)

Server `ts_i386.so` (nm → Ghidra = +0x10000):
- `CTSGun::UpdateClientData` (WeaponInfo/WStatus) — `0x758c8` / `0x858c8`
- `CTSGun::TakeWeapon` — `0x70a04` / `0x80a04`
- `CTSGun::Deploy` — `0x71030` / `0x81030`
- `GetBodyByItems(int,int)` — `0x6f0a4` / `0x7f0a4`
- `CTSGun::AkimboFire` — `0x729c0` / `0x829c0`
- DLL `UpdateClientData` (clientdata pack) — `0xa0294` / `0xb0294`

Client `client.dll` (PE `0x10000000 + RVA`):
- bar draw — `FUN_1004e030` (0x4e030)
- WStatus handler — `FUN_1004c840` → `FUN_1004f990` (0x4c840 / 0x4f990)
- `HUD_WeaponsPostThink` — `FUN_10003830` (0x3830)
- draw.sc event — `FUN_100425b0` (0x425b0); `GetBodyByItems` = `FUN_10007110`

---

## 6. Both-sides fix summary (files + lines)

| Bug | Server | Client |
|---|---|---|
| 1 combine clip | none (`ts_gun.cpp:499-514` already faithful) | none |
| 2 phantom red single | `ts_gun.cpp` `UpdateClientData` (2426): hoist the WStatus owned-loop above the WeaponInfo early-return (2460-2463); change-detect on OWNED; add the paired-variant resend (iAkimboType/iSingleType) after 2477 | `ts_weaponbar.cpp` already faithful (opt: red at `< 1` @388) |
| 3 deploy flicker | none (`ts_gun.cpp:771` Deploy already sets body) | `ts_weapon_predict.cpp:39` EV_TSDraw — set viewmodel deploy anim + `GetBodyByItems(type,attach)` body on the deploy frame (mirror `FUN_100425b0`) |
| 4 vuser4.z / left clip | `dlls/client.cpp:2195-2197` — `cd->ammo_nails = it.leftClip`; `cd->vuser4.z = 0` (item+0x0c, VERIFY) | `hl_weapons.cpp` — read leftClip from `ammo_nails`, `rightClip = clip-leftClip`; pack `ammo_nails = leftClip`; drop the `vuser4.z→leftClip` read (626) and the `ammo_bolts↔ammo_nails` use (677/756) for the tsgun |
