/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

typedef enum
{
	expRandom,
	expDirected
} Explosions;
typedef enum
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matLastMaterial
} Materials;

#define NUM_SHARDS 6 // this many shards spawned when breakable objects break;

class CBreakable : public CBaseDelay
{
public:
	// basic functions
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT BreakTouch(CBaseEntity* pOther);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void DamageSound();

	// breakables use an overridden takedamage
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	// To spark when hit
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	bool IsBreakable();
	bool SparkWhenHit();

	int DamageDecal(int bitsDamageType) override;

	void EXPORT Die();
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	// TS respawn additions (TS_func_break.cpp): a "respawntime"-flagged breakable goes hidden on Die
	// and reappears after the delay (box-clear retry). Killed only attributes the attacker.
	void Killed(entvars_t* pevAttacker, int iGib) override; // @0x652a8 — set the global attacker, Die()
	void BreakableRespawn(bool forceRespawn);               // @0x64e84 — box-clear retry + restore

	bool Explodable();
	int ExplosionMagnitude();
	void ExplosionSetMagnitude(int magnitude);

	static void MaterialSoundPrecache(Materials precacheMaterial);
	static void MaterialSoundRandom(edict_t* pEdict, Materials soundMaterial, float volume);
	static const char** MaterialSoundList(Materials precacheMaterial, int& soundCount);

	static const char* pSoundsWood[];
	static const char* pSoundsFlesh[];
	static const char* pSoundsGlass[];
	static const char* pSoundsMetal[];
	static const char* pSoundsConcrete[];
	static const char* pSpawnObjects[];

	static TYPEDESCRIPTION m_SaveData[];

	Materials m_Material;
	Explosions m_Explosion;
	int m_idShard;
	float m_angle;
	int m_iszGibModel;
	int m_iszSpawnObject;

	// TS respawn state (net-new; not in the save table, matching the original). m_flRespawnTime < 0
	// (the FGD default) = non-respawning, so stock breakables are unaffected.
	float   m_flDieDamage = 0;            // transient — the last TraceAttack damage (shard-speed scale)
	float   m_flRespawnHealth = 0;       // +0x6c — spawn health, restored on respawn
	EHANDLE m_hRespawner;                 // +0x70 — the CInfoBreakable retry-helper (created in Spawn)
	float   m_flRespawnTime = 0;          // +0x90 — keyvalue "respawntime". DEFAULT 0 (gcc2 entity data is
	                                      // calloc'd, so an ABSENT keyvalue = 0 = the 30s default respawn —
	                                      // EVERY breakable respawns unless the map sets it NEGATIVE)
	char    m_szRespawnTarget[64] = {};   // +0x94 — keyvalue "respawntarget" (fired on respawn if SF 8)
};
