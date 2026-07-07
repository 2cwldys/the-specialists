# cont91 — Third-person self-transparency when aiming at a player

**Feature:** In third person, the LOCAL player's own model fades to a semi-transparent
alpha while the aim (crosshair) is on ANOTHER PLAYER, so your own body/shoulder stops
blocking the target. Ours never does this — the local model is always fully opaque in
third person.

**Side: CLIENT (client.dll).** Purely a visual override on the LOCAL player cl_entity's
`curstate.rendermode/renderamt`, recomputed every frame in the refdef path. The SERVER is
**not** involved: `pev->rendermode/renderamt` is never touched for aiming (the only server
render writes are the spawn-protection shimmer in `dlls/ts_player.cpp:684` and its clear in
`dlls/player.cpp:1910`). The studio blend that consumes it is the stock path
`StudioRenderFinal_Hardware` reading `m_pCurrentEntity->curstate.rendermode` (our
`cl_dll/StudioModelRenderer.cpp:1778`).

All addresses are client.dll RVAs (`scripts/ghidra/dc.sh client 0x<rva>`; PE imagebase
0x10000000). Decompile evidence below is decompile-exact except where marked `// VERIFY`.

---

## 1. The exact original behavior

### 1a. The alpha override — `V_CalcNormalRefdef` @ 0x67890 (top of the function)

`V_CalcNormalRefdef` is the client's real refdef builder (its wrapper `V_CalcRefdef` calls it
at 0x69bc5). The FIRST thing it does, every frame, before any bob/camera work:

```c
// decompile @0x678b8, cleaned:
plocal = GetLocalPlayer();          // (*DAT_100d5bf4)()  — see §3 for the id proof
if (FUN_1004c250() == 0) {          // state gate (§1c). 0 => run the fade logic
    target = 255.0f;                // _DAT_1007cae0 (read from file = 255.0)
    if (DAT_1018a60c != 0 && (int)__ftol(/*ST0*/) != 0)   // aiming at a player (§2)
        target = 100.0f;            // _DAT_1007a910 (read from file = 100.0)
    DAT_10097210 = target;          // global "target alpha"
    // approach the current alpha toward target (FUN_10057e90 @0x57e90 = clamp-approach):
    step = (DAT_10097214 >= DAT_10097210) ? 50.0f : 5.0f;   // fast IN, slow OUT (§1b)
    DAT_10097214 = FUN_10057e90(DAT_10097210, DAT_10097214, step);  // current alpha
    plocal->curstate.rendermode = 1;                 // kRenderTransColor  [asm @0x6796c]
    plocal->curstate.renderamt  = (int)DAT_10097214; //                    [asm @0x67984]
} else {
    plocal->curstate.rendermode = 0;    // kRenderNormal   [asm @0x6798c, edi=0]
    plocal->curstate.renderamt  = 0xff; // 255            [asm @0x67992]
}
```

Raw asm anchors (from `objdump -d` of reference/client.dll):
```
1006796c: c7 86 f8 02 00 00 01   mov DWORD PTR [esi+0x2f8],0x1    ; curstate.rendermode = 1
10067984: 89 86 fc 02 00 00      mov DWORD PTR [esi+0x2fc],eax    ; curstate.renderamt  = smoothed
1006798c: 89 be f8 02 00 00      mov DWORD PTR [esi+0x2f8],edi    ; rendermode = 0 (opaque branch)
10067992: c7 86 fc 02 00 00 ff   mov DWORD PTR [esi+0x2fc],0xff   ; renderamt  = 255
```
esi = GetLocalPlayer() return. **Offset proof:** curstate base in cl_entity_t = 0x2b0, so
rendermode = +0x48 = **0x2f8**, renderamt = +0x4c = **0x2fc**, renderfx = +0x54 = 0x304.
Cross-checked: the same block that this renderfx offset (0x304) is written elsewhere with the
kRenderFx enum values 0xe/0x11(kRenderFxDeadPlayer)/0x13(kRenderFxGlowShell), and
StudioDrawPlayer writes controllers at cl_entity+0x318 = curstate+0x68 — both consistent with
curstate @0x2b0.

**Values (float constants read straight out of client.dll .rdata):**
- opaque target `_DAT_1007cae0` @0x7cae0 = **255.0**
- aiming target `_DAT_1007a910` @0x7a910 = **100.0**  ← ~39% opacity while aiming at a player
- rendermode while the fade logic is active = **kRenderTransColor (1)**

Note the model is *always* driven with rendermode `kRenderTransColor` when the fade logic is
active; when not aiming the alpha simply eases back to 255 (opaque-looking). The only visible
transparency is the ease toward 100 while aiming at a player.

### 1b. Smoothing (secondary; `// VERIFY` the exact steps)

`DAT_10097214` (current) is eased toward `DAT_10097210` (target) by `FUN_10057e90(target,cur,
step)` — a plain clamp-approach (moves `cur` toward `target` by at most `step`). The step is
selected: `step = (cur >= target) ? 50 : 5` per frame (the `dVar4 * DAT_100cf950 == 0`
branch; `DAT_100cf950` is a .bss runtime global, non-zero in play). So: **snap toward
transparent (~50/frame), ease back toward opaque (~5/frame)**. No frametime multiply is applied
in the call, so these are per-frame increments. `// VERIFY` the 50/5 exactly against a live
read; not load-bearing for correctness.

### 1c. The state gate `FUN_1004c250` @0x4c250 (`// VERIFY` identity)

```c
uVar1 = (*DAT_100d5c7c[1])();            // fn ptr in an API struct @0xd5c78/0xd5c7c/0xd5c80
if (uVar1 == 0) { uVar1 = (*DAT_100d5c7c[0])();
    if (uVar1 == 0) return (DAT_100d43ec == 0); }   // low byte 1 => OPAQUE branch
return uVar1 & 0xffffff00;                // low byte 0 => run fade logic
```
Caller tests the LOW byte (`(char)ret == 0` => fade logic). Net: the fade logic runs unless
`fn0()==0 && fn1()==0 && DAT_100d43ec==0` (the "opaque hard-reset" state). This is a
spectator/demo/paused-style guard; `fn0/fn1` are two predicates from the API struct at
0xd5c78 (same struct the HUD-message/demo code uses at 0x209f0/0x21efe). **Exact identity not
pinned** — `// VERIFY` (candidates: IsSpectateOnly / demo IsPlayingback / third-person
predicate). It is NOT load-bearing for the port: functionally it just forces rendermode=0 /
renderamt=255 in those edge states, which our reconstruction can reproduce with our existing
spectator/dead checks.

---

## 2. The trigger flag `DAT_1018a60c` — aim-at-player classifier `FUN_1004acc0` @0x4acc0

`DAT_1018a60c` is a tri-state byte, written only by `FUN_1004acc0` (called once per frame from
`FUN_10049f78`, a HUD/reticle update):

- `0x4adf5: mov byte [0x1018a60c],0` — nothing / world under the aim
- `0x4af1a: mov byte [0x1018a60c],1` — a player, SAME team  (`// VERIFY` which is 1 vs 2)
- `0x4ae85: mov byte [0x1018a60c],2` — a player, DIFFERENT team

Mechanism (decompile @0x4acc0):
1. `plocal = GetLocalPlayer()` (DAT_100d5bf4); save its index.
2. PM trace along the aim using the trace API struct at `0x100d5c78` (`[+0x28]/[+0x2c]` =
   trace setup + trace-line).
3. `hit = GetEntityByIndex(traceResult)` (DAT_100d5bfc @0x4ade5); if `hit==0` → flag 0.
4. If `hit` is a player, compare its team word against the local team global
   (`cmp cx, WORD [edx+0x101901fc]` @0x4aeaa): same team → flag 1, different → flag 2.

This is the SAME detection that drives the enemy/team reticle sprite swap
(cross_e / cross_t; reader at ~0x4eac0/0x4eb3f). **The transparency in §1 fires on ANY
non-zero value** (`if (DAT_1018a60c != 0)`), i.e. aiming at a teammate OR an enemy — despite
the "enemies" framing, it is "aiming at a player." There is an additional `__ftol()!=0`
sub-gate (a cvar/state read whose source Ghidra lost) — `// VERIFY`; likely a feature toggle,
default on.

---

## 3. Identity proofs

- `DAT_100d5bf4()` = **GetLocalPlayer**. Proof: `DAT_100d5bfc` is called as
  `GetEntityByIndex(idx)` in both `FUN_1004acc0` and `V_CalcNormalRefdef`
  (`(*DAT_100d5bfc)(pparams->viewentity)`); these three consecutive cl_enginefunc_t pointers
  are `...bf4 = GetLocalPlayer, ...bf8 = GetViewModel, ...bfc = GetEntityByIndex` (the SDK
  order), so the entity being faded is the local player, not the viewmodel.
- Offsets 0x2f8 / 0x2fc = curstate.rendermode / curstate.renderamt (see §1a proof).

---

## 4. What OURS is missing

- `cl_dll/view.cpp` `V_CalcNormalRefdef` (line 640; gets `ent = GetLocalPlayer()` at line 667)
  builds the third-person camera (lines ~806-841) but NEVER writes
  `GetLocalPlayer()->curstate.rendermode/renderamt`. → local model always opaque.
- `cl_dll/ts_reticle.cpp` already does an aim `PM_TraceLine(v_aimeye, eye+fwd*8192, 0, 2, -1)`
  (lines ~205-215) but only uses `endpos` for the floating crosshair — it does NOT classify
  the hit entity as a player, and there is no `DAT_1018a60c` equivalent / no transparency.
- No server involvement needed or present (correct — the feature is client-only).

---

## 5. Concrete fix spec (client-only, no re-RE needed)

**Step A — aim-at-player classifier (new small module-global, fed from the existing trace).**
In `cl_dll/ts_reticle.cpp` (or a tiny new `ts_aim.cpp`), each frame compute:

```c
// 0 = none, 1 = teammate under aim, 2 = enemy under aim  (transparency fires on !=0)
int g_iTSAimTarget = 0;
void TS_UpdateAimTarget() {
    g_iTSAimTarget = 0;
    Vector va; gEngfuncs.GetViewAngles(va);
    Vector fwd; gEngfuncs.pfnAngleVectors(va, fwd, NULL, NULL);
    Vector eye = v_aimeye;                 // already exported by view.cpp (bullet origin)
    Vector end = eye + fwd * 8192.0f;
    pmtrace_t* tr = gEngfuncs.PM_TraceLine(eye, end, 0 /*PM_NORMAL*/, 2 /*studio*/, -1);
    if (!tr || tr->fraction >= 1.0f) return;
    int ent = gEngfuncs.PM_GetPhysent(tr->ent) ? tr->ent : -1;   // or use tr->ent index
    cl_entity_t* hit = (ent > 0) ? gEngfuncs.GetEntityByIndex(ent) : NULL;
    if (!hit || !hit->player) return;
    // team compare: g_iTeamNumber (local, set in entity.cpp:153) vs the hit player's team.
    extern int g_iTeamNumber;
    int hitTeam = /* hit player's team; if unavailable, treat all players as enemies */;
    g_iTSAimTarget = (hitTeam == g_iTeamNumber) ? 1 : 2;   // both != 0 => fade
}
```
`// VERIFY` the exact `tr->ent`→entity-index mapping under our engine (xash/rehlds); the
original used its PM trace-result index straight into GetEntityByIndex. If per-player team is
awkward client-side, classifying any hit player as target (== original firing on `!=0`) is
faithful to the transparency behavior.

**Step B — apply the alpha in `cl_dll/view.cpp` `V_CalcNormalRefdef`.** Insert right after
`ent = gEngfuncs.GetLocalPlayer();` (line 667), matching the original's placement at the TOP
of the function:

```c
extern int g_iTSAimTarget; extern void TS_UpdateAimTarget();
TS_UpdateAimTarget();
if (cl_entity_t* lp = gEngfuncs.GetLocalPlayer()) {
    static float s_alpha = 255.0f;
    // opaque hard-reset in spectator/dead (the original's FUN_1004c250 gate):
    if (gEngfuncs.IsSpectateOnly() || g_iUser1 != 0 || gHUD.m_Health.m_iHealth <= 0) {
        lp->curstate.rendermode = kRenderNormal; lp->curstate.renderamt = 255; s_alpha = 255.0f;
    } else {
        float target = (g_iTSAimTarget != 0) ? 100.0f : 255.0f;   // <-- values from §1a
        float step   = (s_alpha >= target) ? 50.0f : 5.0f;        // fast in, slow out (§1b)
        if (s_alpha < target) s_alpha = (s_alpha + step > target) ? target : s_alpha + step;
        else                  s_alpha = (s_alpha - step < target) ? target : s_alpha - step;
        lp->curstate.rendermode = kRenderTransColor;              // 1  (§1a)
        lp->curstate.renderamt  = (int)s_alpha;
    }
}
```

**Exact values to hardcode:** rendermode = `kRenderTransColor` (1); aiming alpha = **100**;
non-aiming/opaque alpha = **255**; approach steps ~**50** (toward transparent) / ~**5** (back
to opaque) per frame (`// VERIFY` steps). Trigger = crosshair aim-trace hits any player.

**Caveat to check on our engine:** confirm nothing later in our pipeline clobbers the local
player's `curstate.rendermode/renderamt` after `V_CalcNormalRefdef` (HUD_ProcessPlayerState in
`cl_dll/entity.cpp:123-124` copies the SERVER's render fields into the state each network
frame, but that runs on the network state, not the post-refdef curstate the engine renders —
the original relies on exactly this ordering, writing the local player's curstate every frame
in the refdef pass). If our engine re-derives curstate from the network state after refdef,
move the write into `StudioDrawPlayer` for the local player instead (same values).
