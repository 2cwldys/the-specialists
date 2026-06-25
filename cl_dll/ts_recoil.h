/***
*  ts_recoil.h — The Specialists AIM-recoil (client-side, RE-grounded).
*
*  RE of the original client.dll: on fire FUN_10008230 (@0x8230) adds a per-shot kick to a recoil
*  TARGET accumulator; every frame V_CalcRefdef eases the APPLIED recoil toward the target (FUN_10057ed0
*  @0x57ed0) at rate min(frametime*15, 1) and folds the delta into the view angles. The kick moves the
*  ACTUAL AIM (cl.viewangles) and is PERSISTENT — the crosshair climbs and the player must compensate.
*  This is NOT pev->punchangle (that's a separate, bot-gated visual punch in the original).
*
*  Constants recovered live + from client.dll .rodata (see scratchpad/recoil-live-findings.md):
*    base 0.85, firetype horiz scales, duck 0.775, bullet-time 0.66667, ease rate frametime*15 (cap 1),
*    snap-off when remaining < ~0.1deg. Verified against a live cl.viewangles trajectory.
***/
#ifndef TS_RECOIL_H
#define TS_RECOIL_H

struct ref_params_s;

// Called on the LOCAL player's shot (from EV_TSFire). flViewKick = weapon field 0x30, iFireType = 0x348.
void TS_AddViewRecoil(float flViewKick, int iFireType, bool bDuck, bool bBulletTime);

// Called once per frame from V_CalcNormalRefdef: eases the recoil into the aim (persistent).
void TS_UpdateViewRecoil(struct ref_params_s* pparams);

// Trigger the transient view "bounce-back" shake (knife/katana swing AND kung-fu punch/kick). Caller passes
// the final amplitude (degrees): knife/katana = flDamage*0.15; kung-fu = min(|velocity|*0.15385, 45)*0.35.
// A damped oscillation on the RENDER view only (decays to 0, returns to origin) — never the aim.
void TS_TriggerViewShake(float amplitude);

// Called once per frame from V_CalcNormalRefdef (after the render viewangles are seeded): advances + applies
// the transient melee shake to pparams->viewangles only (never the aim).
void TS_ApplyMeleeShake(struct ref_params_s* pparams);

#endif // TS_RECOIL_H
