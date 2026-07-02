/***
*  ts_gun_stubs.cpp — CLIENT-side stubs for the server-only symbols CTSGun references.
*
*  Step 3a of making CTSGun client-predicted (reference/EVENTS.md): dlls/ts_gun.cpp is now
*  compiled into client.so too (like the HL SDK weapon .cpp files) so the client can run
*  CTSGun's clip/timer/anim logic each prediction frame and the predicted viewmodel anim
*  sticks. ts_gun.cpp COMPILES client-side unchanged; it only fails to LINK against a handful
*  of genuinely server-only functions (entity creation, the 3rd-person CTSAnimation, the map
*  precache, the powerup query). Those are server-authoritative side-effects the prediction
*  must NOT perform, so here they are no-ops on the client. Zero edits to the verified
*  ts_gun.cpp — the real clip/timer/SendWeaponAnim/event logic runs; only the side-effects
*  are stubbed.
*
*  (None of CTSGun's server-only standalone helpers — drop/pickup/death scatter — are reached
*  during prediction; they link here as dead code via these stubs.)
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_animation.h"
#include "ts_player.h"

// --- 3rd-person player animation (ts_animation.cpp, server-only) — the client doesn't drive
// other players' gait from prediction; the server replicates it. No-op client-side.
void CTSAnimation::Animate(CBasePlayer *, int) {}

// --- powerup query (ts_player.cpp): the double-fire-rate powerup isn't shipped to client
// prediction yet, so assume "off". VERIFY: predicted fire rate ignores the powerup until its
// flag is mirrored via weapon_data (step 3c). First-cut acceptable (powerup is transient).
bool DoubleFireRate(CBasePlayer *) { return false; }

// --- map-time precache (ts_weapons.cpp, server-only). The client precaches via its own paths.
void PrecacheAllTSWeapons() {}

// --- world-entity spawners (server-only): thrown grenades / knives are created authoritatively by
// the server. Never reached during prediction. (Dropped guns/ammo packs are now spawned via the
// static CTSWorldGun::CreateWeapon / CTSAmmoPack::CreateAmmoPack factories, whose only callers — the
// drop helpers in ts_gun.cpp — are #ifndef CLIENT_DLL, so they need no client stub.)
CBaseEntity *TSThrowM61(entvars_s *, const Vector &, const Vector &, float) { return nullptr; }
CBaseEntity *TSThrowKnife(CBasePlayer *, const Vector &, const Vector &, int) { return nullptr; }

// --- powerup query (ts_player.cpp): infinite-clips isn't mirrored into prediction; assume "off"
// (same accepted tradeoff as DoubleFireRate above — a mispredicted consume resyncs from the server).
bool InfiniteClips(CBasePlayer *) { return false; }

// --- server-side movement-speed push (ts_player.cpp): prediction doesn't drive maxspeed; no-op.
void CBasePlayer::SetPlayerSpeed(float) {}

// --- engine entity glue (server-only; used only by CTSGun's drop/pickup helpers).
int DispatchSpawn(edict_t *) { return 0; }
void DispatchTouch(edict_t *, edict_t *) {}
