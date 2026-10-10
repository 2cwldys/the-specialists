/***
*  ts_gun_stubs.cpp — the client's versions of the server-only functions CTSGun references.
*
*  dlls/ts_gun.cpp is compiled into client.so too (like the HL SDK weapon .cpp files) so the
*  client predicts CTSGun's clip, timers and animations. The server-only functions it calls
*  (entity creation, the third-person CTSAnimation, the map precache, engine entity glue) do
*  nothing here: those side effects are the server's. The powerup queries are real, as in
*  client.dll. CTSGun's drop, pickup and death-scatter helpers are never reached by the
*  prediction.
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

// --- powerup queries: client.dll has its own (DoubleFireRate 0x10007100 and InfiniteClips
// 0x100070f0), reading the predicted player's powerup bits, which HUD_WeaponsPostThink takes from
// clientdata vuser1.y.
bool DoubleFireRate(CBasePlayer *p) { return (p->m_iPowerups & 0x10) != 0; }
bool InfiniteClips(CBasePlayer *p) { return (p->m_iPowerups & 2) != 0; }

// --- map-time precache (ts_weapons.cpp, server-only). The client precaches via its own paths.
void PrecacheAllTSWeapons() {}

// --- world-entity spawners (server-only): thrown grenades / knives are created authoritatively by
// the server. Never reached during prediction. (Dropped guns/ammo packs are now spawned via the
// static CTSWorldGun::CreateWeapon / CTSAmmoPack::CreateAmmoPack factories, whose only callers — the
// drop helpers in ts_gun.cpp — are #ifndef CLIENT_DLL, so they need no client version.)
CBaseEntity *TSThrowM61(entvars_s *, const Vector &, const Vector &, float) { return nullptr; }
CBaseEntity *TSThrowKnife(CBasePlayer *, const Vector &, const Vector &, int) { return nullptr; }

// --- server-side movement-speed push (ts_player.cpp): prediction doesn't drive maxspeed; no-op.
void CBasePlayer::SetPlayerSpeed(float) {}

// --- engine entity glue (server-only; used only by CTSGun's drop/pickup helpers).
int DispatchSpawn(edict_t *) { return 0; }
void DispatchTouch(edict_t *, edict_t *) {}
