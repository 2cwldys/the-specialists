# ★ TSState / TSSpace / TSBTime — wire, gates, source members (SETTLED)

Ground truth: typed `re` decompile of `CBasePlayer::UpdateClientData` @ nm **0x81d00**
(`star_decomp.txt` lines 213–430, TS_player.cpp). All three sends live there, inside the
function-wide throttle:

```c
// UpdateClientData runs its whole body at most every 0.1 s:
if (player+0x534 (float m_flNextUCDTime) <= gpGlobals->time && 0 <= it) {
    player+0x534 = gpGlobals->time + 0.1;    // dc_0_1
    ... all sends ...
}
```
Send order inside the function: ResetHUD/InitHUD block → RoundTime → PwUp(TheOne re-push) →
HideWeapon → TSHealth → TSArmor → Train → **TSState** → **TSCash** → **TSSpace** → PwUp →
TSSlowMo → **TSBTime** → ActItems → **TSPAward** → **Wires** → active-item/FOV caches.

## TSState ("TSState", 1)

```c
if (this->field_0x498 != (char)this->field_0x499) {   // source vs cache
    this->field_0x499 = this->field_0x498;            // cache = last sent
    MESSAGE_BEGIN(MSG_ONE, gmsgTSState, NULL, edict);
    WRITE_BYTE((byte)this->field_0x498);
    MESSAGE_END();
}
```
- **Source = byte @0x498** (our `m_iPlayerState`, fed by `UpdatePlayerState()`, which PreThink
  calls right before this — confirmed at PreThink +0xb46, 0x81416). **Cache = byte @0x499**
  (dossier's "plausible 0x499" CONFIRMED).
- MSG_ONE confirmed (dest=1). On-change only.
- **No ResetHUD-forced resend**: the InitHUD path does `memset(&player->0x49A, 0, 0x40)` —
  starts at 0x49A, deliberately (or luckily) NOT clearing the 0x499 cache. Reconstruct as-is:
  plain on-change, no forced resend (new player objects zero both, so a joiner starts consistent).

## TSSpace ("TSSpace", 1)

```c
if (this->field_0x548 != this->field_0x54c) {
    this->field_0x54c = this->field_0x548;
    MESSAGE_BEGIN(MSG_ONE, gmsgTSSpace, NULL, edict);
    WRITE_BYTE(this->field_0x548);
    MESSAGE_END();
}
```
- **Source = int @0x548 read DIRECTLY** — no recompute, **no clamp at the WRITE_BYTE**
  (VERIFY item settled: it does NOT call TSPlayerFreeSlots at send time; +0x548 is the maintained
  free-slots int; our reconstruction must keep +0x548 up to date and send the member, or ensure
  `TSPlayerFreeSlots()` result is always stored there). **Cache = int @0x54c.** MSG_ONE.

## TSBTime ("TSBTime", 1) — the 0x175-vs-[0x5d] ambiguity SETTLED

```c
if (this->field_0x174 != this->field_0x175) {
    this->field_0x174 = this->field_0x175;
    MESSAGE_BEGIN(MSG_ONE, gmsgTSBulletTime, NULL, edict);
    WRITE_BYTE((char)this->field_0x174);
    MESSAGE_END();
}
```
- **Source = byte @0x175. Change-detect cache = byte @0x174.** HUD_MESSAGES' `[0x5d]` was the
  dword-index rendering of the same 0x174/0x175 slot. Our `m_iClientBulletTime // 0x174`
  (player.h:414) is exactly the cache — absorb it (as a byte-semantics int).
- **The 0x175 byte IS the byte our repo calls `m_bSlowBullets`** — one member, and it is
  precisely the slow-bullets/bullet-time variant flag. Complete writer/reader inventory
  (objdump scan of `0x175(%e`):
  - `CBasePlayer::GoSlow(float amt, char bt)` @0x829d0: `0x175 = bt` (2nd param), plus
    m_flSlowMotionGo/Target = amt (Go forced to 1.0 when observer iuser1!=0). Decompiled
    (`star_decomp2.txt`).
  - `CBasePlayer::UpdatePowerUpState` @0x85748: powerup&8 (bullet-time powerup) → `0x175=1`,
    ramp 0.225, scale 0.8; else powerup&1 (slow-mo powerup) → `0x175=0`, ramp 0.35, scale 0.6.
  - `CBasePlayer::DeactivateSlowMotionFlow` @0x86a9c: `0x175=0` (two sites).
  - `CBasePlayer::SlowMotionFlow` @0x86bb0: copies/propagates it (read ×2, write ×1).
  - `CTSGameRules::GlobalSlowMotion2` @0x69720 (+0xbd, `setne`): sets it per player from the
    global slow-mo flag.
  - `CTSSlowMotion::WaitTouch` @0x60fdc, `CTSSlowMotionPoint::WaitThink` @0x61728 (`setne`).
  - `CBasePlayer::TSInit` @0x82a4c (+0x61): `0x175=0` (spawn-init clear).
  - Reader: `CBulletManager::UpdateBullet` @0x5ef5c (the slow-bullets consumer).
- So: do NOT invent a second member. Send `m_bSlowBullets` (rename/alias per audit naming),
  cache in `m_iClientBulletTime`. Client stores `READ_BYTE() != 0` → recoil ×0.66667 consumer
  (ev_ts.cpp:346 hardcoded `false` can now be wired).
- The dossier's caution "do not conflate with m_bSlowBullets" resolves the OPPOSITE way:
  they are the SAME byte, by construction.
