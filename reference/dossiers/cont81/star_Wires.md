# ★ Wires — CheckForWires + the 3-byte encoding + callers (SETTLED)

Ground truth: typed decompiles of `CBasePlayer::CheckForWires` @ nm **0x82f08**,
`CBasePlayer::UpdateClientData` @0x81d00 (`star_decomp.txt`), `CTSBomb::SetRandomWires` @0x7c598,
`CTSBomb::CutWire` @0x7c70c (`star_decomp2.txt`), plus objdump of `CBasePlayer::CutWire` @0x82fb4
and the sole CheckForWires call site.

## CBasePlayer::CheckForWires (TS_player.cpp) — the encoder

```c
void CBasePlayer::CheckForWires() {
    field_0x78c = field_0x78d = field_0x78e = 0;            // clear-on-walk-away = zeros ✓
    CBaseEntity *e = NULL;
    while ((e = UTIL_FindEntityInSphere(e, pev->origin, 64.0)) != NULL) {
        if (!strcmp(STRING(e->pev->classname), "ts_bomb")
            && ((CTSBomb*)e)->m_bActive /*+0x64*/
            && ((CTSObjective*)e)->CanInteract(this)) {
            field_0x78c = bomb->0x7c;   // wire[0]
            field_0x78d = bomb->0x7d;   // wire[1]
            field_0x78e = bomb->0x7e;   // wire[2]
        }   // no break — LAST matching bomb in the sphere wins
    }
}
```
- Trigger condition = sphere radius **64**, classname `ts_bomb`, bomb **active** (+0x64, our
  `m_bActive`, inherited CTSObjective flag), and `CTSObjective::CanInteract(player)` (team gate).
  NOT gated on `cancutwire` — a non-cuttable active bomb still shows its wires.

## The sole caller — PreThink (this is the missing-method wiring)

Objdump 0x81412–0x81455 (PreThink +0xb46..+0xb89), right AFTER `UpdatePlayerState(this)`:

```c
UpdatePlayerState(this);
if (*(byte*)TSGameRules == 3                       // gamemode byte0 == 3 (bomb/objective mode)
    && this->field_0x794 < gpGlobals->time) {      // per-player throttle timer
    this->field_0x794 = gpGlobals->time + 0.5;     // double 0.5 @ .rodata 0xf6e00
    CheckForWires(this);
}
```
- **Called from PreThink only**, gamemode==3 only, throttled to **every 0.5 s** via transient
  float @player+0x794. (`re.sh refs` not needed — objdump shows exactly one call, 0x81450.)

## The send — UpdateClientData (on-change, MSG_ONE)

```c
if (0x78c != 0x78f || *(short*)&0x78d != (short)dword@0x790) {  // triple byte compare
    0x78f = 0x78c; 0x790 = 0x78d; 0x791 = 0x78e;                // cache = last-sent triple
    MESSAGE_BEGIN(MSG_ONE, gmsgTSWires, NULL, edict);
    WRITE_BYTE(0x78c); WRITE_BYTE(0x78d); WRITE_BYTE(0x78e);
    MESSAGE_END();
}
```
Source triple @0x78c–0x78e, **cache triple @0x78f–0x791**. Registered `"Wires"` 3, order between
ShowObj and SrvSett (see star_LinkUserMessages.md).

## Byte VALUES — the wire ROLE codes go on the wire (defuse-leak question answered)

`SetRandomWires`: `{3,3,3}`, then one random index → **1**, then a different random index → **2**.
So the sent bytes are: **0 = cut, 1 = defuse wire, 2 = boobytrap wire, 3 = neutral/intact**.
**The original DOES leak the roles to the owning client** — CheckForWires copies the bomb's role
bytes verbatim. The stock client renders them opaquely (any nonzero = intact wire in the cut UI),
but the info is in the packet. Drop-in fidelity = reproduce exactly (do not "fix" the leak).

## CTSBomb::CutWire(int i, CBasePlayer*) — role semantics (cross-check)

Gate: `m_bActive && cancutwire && field_0x6c > 0 && i < 3 && wire[i] != 0 && player != NULL`.
- wire==**3** (neutral): `wire[i]=0; field_0x6c=0;` (kills the cut window/timer — one shot).
- wire==**2** (boobytrap): `field_0x7f=1` (our `m_bFast` — speeds the countdown), `wire[i]=0`, and
  **immediately re-syncs the CUTTER's player triple** (0x78c–e = bomb wires) so the HUD updates
  before the next 0.5 s CheckForWires tick.
- wire==**1** (defuse): `wire[i]=0`; FireTargets(pev->target); `TSHudMessageAll(0,255,0,8,30,
  pev->message)` (green); `field_0x6c=0; field_0x70=0; Freeze(true)` — bomb defused.

## CBasePlayer::CutWire(int) — targeting divergence in OUR code

Original (objdump 0x82fb4): gate `IsAlive() && pev->iuser1 == 0`; then **zeroes the player's own
wire triple** (0x78c–e), then `UTIL_FindEntityInSphere(origin, 64.0)` loop, `strcmp` classname
`ts_bomb`, calling `CTSBomb::CutWire(i, this)` on every ts_bomb in the **64-unit sphere**.
**Our all-bombs `FindEntityByClassname` map-wide loop (ts_bomb.cpp:198) DIVERGES — original is
sphere-64-limited** and pre-clears the HUD triple. Fix when landing Wires.

## Reconcile deltas vs dossier plan
- CheckForWires is NOT called from UpdateClientData — add it to **PreThink** (gamemode 3,
  0.5 s throttle member, provenance +0x794), after UpdatePlayerState.
- Cache = +0x78f..0x791 (transient); no forced resend anywhere — map-change reset comes free from
  fresh player objects.
- Client half unchanged from dossier (CHudTSWeaponBar `m_wires[3]` @+0x36fc, FUN_1004d550 draw).
