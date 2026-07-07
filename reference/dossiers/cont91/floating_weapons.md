# cont.91 — Floating / detached held weapons on other players (3rd person)

**Symptom (as reported):** looking at OTHER players in 3rd person, their held weapon is
DETACHED from the hand — floating in mid-air, pointing forward. Claimed PER-WEAPON:
`usas12` attaches, "many others" float.

**Scope of this dossier:** READ-ONLY RE of the per-weapon held-model (p_model) data and the
server-side state that drives the engine's held-weapon attachment. No source edited, no build.

---

## TL;DR — the hypothesized cause is DISPROVEN by the evidence

The bug brief hypothesized one of: (a) a wrong/empty `p_model` string per weapon, (b) a wrong
`w_model`/`body`/model-index, or (c) a single shared p-model with per-weapon BODYGROUPS where a
missing body/skin is unset. **All three are false.** Hard evidence below shows:

1. **Every weapon's `pszPlayerModel` in our table byte-matches the original** (`compile_weapons_info`
   @`0x6fe28`, dumped by `scripts/extract_weapons.py`). No empty/wrong p_model.
2. **Every `p_*.mdl` referenced exists on disk and is precached** (`TSPrecache` @`0x8f320`,
   `dlls/TS_util.cpp:87-97` walks weapons `0..0x25` and precaches `pszViewModel`,
   `pszPlayerModel`, `pszPlayerModelSil`, `pszWorldModel`).
3. **The p_models are NOT one shared model with bodygroups** — each is a separate file, every one
   with `numbodyparts=1, nummodels=1` (no submodels), so `pev->body` cannot mis-select a weapon
   submodel and cannot cause a float.
4. **Every p_model's bone skeleton is a strict subset of `player.mdl`'s 34 bones**, so the engine's
   `StudioMergeBones` (attach-by-bone-name) attaches ALL of them to the hand identically. There is
   no per-weapon skeleton that would attach for `usas` and float for `beretta` — their skeletons
   are **byte-identical** (md5 of the bone-name list is the same).
5. **`CTSGun::Deploy` (ours) is faithful to the original** (`Deploy__6CTSGun` @`0x71030`): same
   p_model selection (silenced variant iff `attach&1`), same `pev->body = GetBodyByItems(...)`, same
   `DefaultDeploy` args, same anim-override switch (`DefaultDeploy__17CBasePlayerWeapon` @`0x90178`:
   knife/sealknife→`knife_idle`, katana→`katana_idle`, flag→`briefcase_idle`).
6. **The 3rd-person torso sequence path is faithful** (`CTSAnimation::SetAnim` @`0x5d7e4` — the
   original ALSO falls back to sequence 0 when `LookupSequence` returns -1; our `dlls/ts_animation.cpp`
   SetAnim matches bit-for-bit). Grips `onehanded`/`twohanded`/`akimbo` all resolve to real
   sequences (`aim_/idle_/draw_<grip>`) that exist in `player.mdl`.

**Conclusion:** the per-weapon held-model DATA is correct. Do NOT "fix" the `pszPlayerModel`
column — that would be a regression. The one real (minor) data gap found is the missing **pdw**
silenced p-model (§5), which does NOT cause a float. The reported "many weapons float while usas
works" is **not reproducible from static analysis of the weapon-info data or the deploy/animation
reconstruction** — every server-controlled input the engine uses to attach the held model is
faithful. The residual float, if it reproduces, is a RUNTIME/animation-state issue (§6 diagnostic
plan), not a p_model-table issue. This is the honest, evidence-backed finding and it contradicts
the brief's premise.

---

## 1. How a held weapon attaches (mechanism) — where "floating, pointing forward" comes from

The engine draws another player's held weapon as the player's `pev->weaponmodel` (the `p_*.mdl`),
merged onto the player skeleton. In the drop-in target this is the **original `client.dll`**;
our `cl_dll/StudioModelRenderer.cpp:1634-1653` reconstructs the same path:

```
if (pplayer->weaponmodel) {
    pweaponmodel = GetModelByIndex(pplayer->weaponmodel);
    m_pStudioHeader = Mod_Extradata(pweaponmodel);
    StudioMergeBones(pweaponmodel);   // <-- attach by bone NAME
    StudioRenderModel();
}
```

`StudioMergeBones` (`cl_dll/StudioModelRenderer.cpp:1088-1166`): for each **weapon** bone, if its
name matches a cached **player** bone (`stricmp`), it copies the player's live bone transform onto
the weapon bone; otherwise it poses that weapon bone from the weapon's own idle at the weapon's
reference position.

Two facts make this decisive:

- The weapon is skinned entirely to bones whose names ARE in `player.mdl` (see §3/§4), so **all**
  weapon bones inherit the player's arm/hand transform → the weapon rides the hand. This is true
  for *every* p_model in the roster, including the ones reported to float.
- **"Floating, pointing forward" is the signature of the player's ARM being at the reference/bind
  pose**, i.e. the player's `pev->sequence` is the `empty` sequence (index 0). The weapon dutifully
  attaches to a hand that is in the T/forward reference pose, so it appears detached and forward.
  This is set by the SERVER (`pev->sequence`), not by the p_model.

So the float is governed by the player's networked **sequence**, not by the p_model string/body.

## 2. Per-weapon p_model parity — OUR table vs the ORIGINAL

Source: our `dlls/ts_weapon_info.cpp` `compile_weapons_info()` + `kSil[]`; original from
`scripts/extract_weapons.py` (reads `Write_<weapon>` @ the `compile_weapons_info` @`0x6fe28` body).
`✓` = matches original. All p_models verified present under
`/home/stevenlafl/Containers/hlds/mods/ts/models/`.

| idx | weapon | our pszPlayerModel | our pszPlayerModelSil | grip (animExt) | orig p_model | match |
|----:|--------|--------------------|-----------------------|----------------|--------------|:----:|
| 0 | melee/kungfu | (null) | – | "" | (null) | ✓ (bare hands, no held model) |
| 1 | glock18 | p_glock18 | p_glock18_sil | onehanded | p_glock18 | ✓ |
| 2 | beretta | p_beretta | p_beretta_sil | onehanded | p_beretta | ✓ |
| 3 | uzi | p_uzi | p_uzi_sil | onehanded | p_uzi | ✓ |
| 4 | benellim3 | p_m3 | – | twohanded | p_m3 | ✓ |
| 5 | m4 | p_m4 | p_m4_sil | twohanded | p_m4 | ✓ |
| 6 | mp5sd | p_mp5sd | p_mp5sd | twohanded | (null)→sil p_mp5sd | ✓ (integral silencer, always uses sil slot) |
| 7 | mp5k | p_mp5k | p_mp5k_sil | twohanded | p_mp5k | ✓ |
| 8 | beretta_akimbo | p_berettas | p_berettas_sil | akimbo | p_berettas | ✓ |
| 9 | mk23 | p_mk23 | p_mk23_sil | onehanded | p_mk23 | ✓ |
| 10 | mk23_akimbo | p_mk23_akimbo | p_mk23_akimbo_sil | akimbo | p_mk23_akimbo | ✓ |
| 11 | **usas12 (works)** | p_usas | – | twohanded | p_usas | ✓ |
| 12 | desert | p_desert | p_desert_sil | onehanded | p_desert | ✓ |
| 13 | ak47 | p_ak47 | – | twohanded | p_ak47 | ✓ |
| 14 | fnh | p_fnh | p_fnh_sil | onehanded | p_fnh | ✓ |
| 15 | aug | p_aug | p_aug_sil | twohanded | p_aug | ✓ |
| 16 | uzi_akimbo | p_uzi_akimbo | p_uzi_akimbo_sil | akimbo | p_uzi_akimbo | ✓ |
| 17 | skorpion | p_skorpion | p_tmp_sil | onehanded | p_skorpion | ✓ |
| 18 | barrett | p_m82 | – | twohanded | p_m82 | ✓ |
| 19 | **pdw** | p_pdw | **(null — GAP)** | onehanded | p_pdw | p_model ✓, **sil MISSING** (§5) |
| 20 | spas12 | p_spas12 | – | twohanded | p_spas12 | ✓ |
| 21 | gold | p_gold | – | akimbo | p_gold | ✓ |
| 22 | glock22 | p_glock22 | p_glock22_sil | onehanded | p_glock22 | ✓ |
| 23 | mac10/ump | p_ump | p_ump_sil | twohanded | p_ump | ✓ |
| 24 | m61 grenade | p_m61 | – | **""** | p_m61 | ✓ (see §4 — empty grip) |
| 25 | knife | p_knife | – | "" (knife_idle) | p_knife | ✓ (LEFT-hand model) |
| 26 | mossberg | p_mossberg | – | twohanded | p_mossberg | ✓ |
| 27 | m16 | p_m16 | – | twohanded | p_m16 | ✓ |
| 28 | ruger | p_ruger | p_ruger | onehanded | (null)→sil p_ruger | ✓ (integral silencer) |
| 29 | flag/briefcase | p_knife | – | "" (briefcase_idle) | p_knife (placeholder) | ✓ |
| 30 | fnh_akimbo | p_fnh_akimbo | p_fnh_akimbo_sil | akimbo | p_fnh_akimbo | ✓ |
| 31 | bull | p_bull | – | onehanded | p_bull | ✓ |
| 32 | m60 | p_m60 | – | twohanded | p_m60 | ✓ |
| 33 | sawedoff | p_sawedoff | – | twohanded | p_sawedoff | ✓ |
| 34 | katana | p_katana | – | "" (katana_idle) | p_katana | ✓ |
| 35 | sealknife | p_sealknife | – | "" (knife_idle) | p_sealknife | ✓ (LEFT-hand model) |
| 36 | contender | p_contender | – | onehanded | p_contender | ✓ |
| 37 | skorpion_akimbo | p_skorpion_ak | – | akimbo | p_skorpion_ak | ✓ |

**Every held p_model matches the original.** `usas12` is not special — its p_model, skeleton,
body, and grip are structurally identical to the other twohanded weapons. There is nothing in this
table that would attach `usas` and float `beretta`.

## 3. Model-header evidence (why the p_model can't be the float)

Read via python `studiohdr` parse of the files under `.../mods/ts/models/`:

- All 54 `p_*.mdl`: `numbodyparts=1, nummodels=1, numattachments=0, numseq=1 (idle)`. → No
  bodygroups, so `pev->body` selects nothing on the weapon; a wrong body cannot float it.
- Exactly **three** skeleton shapes, all subsets of `player.mdl`:
  - **11-bone right-arm chain** (`Bip01 … Bip01 DX Mano`) — md5 `1eabbcf3` — the vast majority
    incl. **both `p_usas.mdl` (works) and `p_beretta.mdl` (reported float)** — IDENTICAL.
  - **15-bone dual-arm** (adds `Bip01 SX …`) — md5 `2977d3c2` — the akimbo p_models.
  - **11-bone LEFT-arm chain** (`… Bip01 SX Mano`) — md5 `38f2c26d` — only `p_knife.mdl` &
    `p_sealknife.mdl` (held in the left hand).
- `player.mdl`: `numbones=34`, bones include `Bip01`, `Bip01 DX/SX Clavicola/Braccio superiore/
  Avambraccio/Mano`, etc. — a **superset** of all three weapon skeletons. So `StudioMergeBones`
  finds every weapon bone by name and attaches it. (`models/player/*/*.mdl` remap variants carry
  the same 34-bone skeleton, verified on `agent.mdl`.)

**Implication:** merge attaches every roster weapon identically. A per-weapon float from the model
data is impossible with these files + this merge.

## 4. The one genuine "float-forward" trigger (and it is FAITHFUL, not a bug)

The held weapon points forward when the player's `pev->sequence` = 0 (`empty` = bind pose). That
happens when `CTSAnimation` builds a torso sequence name that does NOT exist in `player.mdl`:
`SetAnim` (@`0x5d7e4`, our `dlls/ts_animation.cpp:79-104`) does `seq = LookupSequence(name); if
(seq==-1) seq=0;` then `pev->sequence = seq`. **The original does exactly this** (decompile:
`uVar2 = LookupSequence(...); uVar2 = uVar2 & ((uVar2==0xffffffff)-1);` → 0 on not-found).

Grip→sequence resolution for the roster:
- `onehanded`/`twohanded`/`akimbo` → `aim_<grip>`/`idle_<grip>`/`draw_<grip>` — **all present** in
  `player.mdl` → arm posed → weapon in hand. This covers **every firearm**, so no firearm floats.
- Empty grip `""`:
  - knife(25)/sealknife(35)/katana(34)/flag(29) set `m_szAnimOverride` (`knife_idle`/`katana_idle`/
    `briefcase_idle`) → those sequences exist → posed correctly.
  - **m61 grenade (24)**: empty grip, NO override, not kung-fu → `aim_`+`""` = `"aim_"` →
    `LookupSequence("aim_")` = -1 → **seq 0 → grenade sits at bind/forward.** But `Write_m61`'s
    `szAnimExt` is empty in the ORIGINAL too, and the original DefaultDeploy (@`0x90178`) does NOT
    add an override for type 24 — so **m61 renders at the same bind pose in the original**. This is
    faithful behaviour, not our regression. // VERIFY at runtime whether TS's m61 visibly "floats"
    (likely it's just a low/reference grenade grip that reads as fine in-game).

So static analysis yields **zero firearms that float** and one grenade that is faithful. This does
not match "many weapons float," which means the reported repro is driven by something outside the
weapon-info/deploy data — see §6.

## 5. Real (minor) divergence found — pdw silenced p-model gap  (NOT the float)

- `dlls/ts_weapon_info.cpp` `kSil[]` (lines ~472-482) has **no entry for pdw (idx 19)**, yet
  `kAttachSupported[19] = 0xB` includes the silencer bit (1), and the asset
  `models/p_pdw_Sil.mdl` (note the capital `S`) ships on disk (93,936 bytes).
- Effect: when a silencer is mounted on the PDW, `CTSGun::Deploy` (`dlls/ts_gun.cpp:796-798`) finds
  `wi->pszPlayerModelSil == nullptr` and keeps the un-silenced `p_pdw.mdl` — so a silenced PDW
  shows the wrong (bare) barrel in 3rd person. **It still attaches** (valid model) — this is a
  cosmetic parity miss, not the float.
- **Fix spec (concrete):** in `dlls/ts_weapon_info.cpp`, inside the `kSil[]` initializer in
  `compile_weapons_info()`, add:
  ```c
  {19, "models/p_pdw_Sil.mdl"},   // pdw silenced held model (disk name has capital 'S')
  ```
  // VERIFY the exact path string against `Write_pdw` in the original (the on-disk file is
  `p_pdw_Sil.mdl`; confirm the original references that same capitalization, since Linux is
  case-sensitive). Also add the same to the client copy if the client mirrors `kSil` separately.

## 6. The reported per-weapon float — diagnostic plan (requires RUNTIME; static analysis exhausted)

Because the p_model data, precache, `Deploy`, `pev->body`, the anim-override switch, and
`SetAnim`/grip→sequence are ALL faithful, a reproduced "many float / usas works" points at the
networked **player animation state** for REMOTE players, not the weapon table. Check, in order:

1. **Remote `pev->sequence`.** Confirm the server runs `CTSAnimation::Animate` every frame for
   ALL players (it is called from PostThink: `dlls/player.cpp:2940/2958-2962`) and that
   `pev->sequence` for a remote player holding a firearm is a real `aim_/idle_<grip>` index (NOT 0).
   If it is 0 for the floaters, the arm is at bind → the (correctly attached) weapon points forward.
   // VERIFY with a server-side dump of `pev->sequence` + `m_szAnimExtention` per player per weapon.
2. **Server model / LookupSequence target.** `LookupSequence` resolves names against the entity's
   *current* studio model. If a remote player's server-side model is not the studio `player.mdl`
   (or the sequence table differs), `aim_onehanded`/`aim_akimbo` resolve to -1 → seq 0 → float.
   `usas` "working" could be coincidental grip/state, or the test player had a valid pose that
   frame. // VERIFY the server `SET_MODEL(player, "models/player.mdl")` path and that indices agree
   with the client's `player.mdl`.
3. **Is the repro on the ORIGINAL client or our `cl_dll`?** In the drop-in target the renderer is
   the original `client.dll`; our `cl_dll/StudioModelRenderer.cpp` only matters if the repro used
   the reconstructed client. The merge in both attaches all skeletons (§1/§3), so this is unlikely
   to be per-weapon — but rule it out by naming the client used for the repro.
4. **Grip correctness on the wire.** Dump `m_szAnimExtention` for each floater at repro time; if
   any firearm shows an empty/garbage grip (it should be onehanded/twohanded/akimbo per §2), trace
   why `DefaultDeploy`'s `strcpy(m_szAnimExtention, szAnimExt)` (`dlls/weapons.cpp:814`) received a
   bad value for that weapon.

**Do not change the `pszPlayerModel` column or `pev->body` — they are proven correct.** The only
weapon-info edit warranted by this investigation is the pdw sil model in §5.

---

## Provenance / commands used
- `scripts/extract_weapons.py` — original weapon table dump (p_model column).
- `scripts/ghidra/dc.sh server 0x71030` — `Deploy__6CTSGun` (p_model + body + DefaultDeploy args).
- `scripts/ghidra/dc.sh server 0x90178` — `DefaultDeploy__17CBasePlayerWeapon` (anim-override switch).
- `scripts/ghidra/dc.sh server 0x5d7e4` — `SetAnim__12CTSAnimation…` (seq=0-on-missing is original).
- python studiohdr parse of `/home/stevenlafl/Containers/hlds/mods/ts/models/{p_*,player,player/agent/agent}.mdl`
  — bodyparts/attachments/bones/seqs (skeleton md5 grouping).
- Source read: `dlls/ts_weapon_info.cpp` (table+kSil), `dlls/ts_gun.cpp:779-837` (Deploy),
  `dlls/weapons.cpp:806-822` (DefaultDeploy), `dlls/TS_util.cpp:66-120` (TSPrecache),
  `dlls/ts_animation.cpp:79-104,455-854` (SetAnim + torso/gait), `dlls/player.cpp:2938-2963`
  (per-frame Animate), `cl_dll/StudioModelRenderer.cpp:1088-1166,1634-1653` (merge + weapon draw).
