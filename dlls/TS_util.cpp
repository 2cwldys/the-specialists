/***
*  TS_util.cpp — the original's server utility free functions (TS_util.cpp TU).
*
*  Landed by the freefuncs plan (reference/dossiers/cont81/dossier-freefuncs.md, F1/F2):
*  the slowed-array helpers, the weapon-param (CustomWP) pipeline, and the player
*  lifecycle utilities. Functions marked DEAD are unreferenced in the shipped Linux .so
*  (no reloc/PLT/direct call) and are kept compiled-and-uncalled for source parity.
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_weapons.h"
#include "ts_weapon_info.h"
#include "ts_gamerules.h" // TSGameRules (the customweapons flag/absorbs)
#include "ts_utils.h"     // TSHudMessageOne/All

#include <string.h>

// @0x8e944 UTIL_EntInArray — first index of pEnt in pArray[0..nCount), else -1. Used by the
// slowed-entity array bookkeeping (RemoveEntityFromSlowedArray / SlowMotionFlow dedup) once
// the packed +0x208 array model lands.
int UTIL_EntInArray(CBaseEntity **pArray, int nCount, CBaseEntity *pEnt)
{
	for (int i = 0; i < nCount; i++)
	{
		if (pArray[i] == pEnt)
			return i;
	}
	return -1;
}

// @0x8e9c0 WeaponIdFromString — tsweapon_t id whose display name matches (case-insensitive),
// else -1. DEAD in the shipped .so (plausibly a compiled-out console command's helper).
int WeaponIdFromString(char *psz)
{
	for (int id = 0; id < TSW_COUNT; id++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)id);
		if (wi->pszName && strcasecmp(psz, wi->pszName) == 0)
			return id;
	}
	return -1;
}

// @0x8fa64 TSFoundRandomEntity — collect up to 32 entities of the classname, return a random
// one (NULL if none). DEAD in the shipped .so.
CBaseEntity *TSFoundRandomEntity(char *pszClassname)
{
	CBaseEntity *arr[32];
	int n = 0;
	CBaseEntity *p = nullptr;
	while (n < 32 && (p = UTIL_FindEntityByClassname(p, pszClassname)) != nullptr)
		arr[n++] = p;
	if (n == 0)
		return nullptr;
	return arr[RANDOM_LONG(0, n - 1)];
}

// @0x8f320 TSPrecache — the map-window precache of the whole TS asset set (called from
// CWorld::Precache; the original's exact list + order, analysis_ff_util §9). Fixes every
// "late precache" console warning (grenbounce via PrecacheOther("m61_grenade"), the
// explo/explode* boom, splat/die/misc sets). Our weapon-table loop covers the fields our
// struct carries (v/p/w models + fire sound; the original also walks two more model slots
// +0xe8/+0x1b2 our table doesn't have yet — VERIFY/extract later).
void TSPrecache(void)
{
	// 1. the fixed sound block (order preserved)
	static const char *const fixed1[] = {
		"player/superjump-land.wav", "player/land.wav", "player/dive.wav", "player/airwheel.wav",
		"player/headshot.wav", "player/headshot2.wav", "player/kevlarhit.wav", "player/block.wav",
		"player/splat.wav", "player/bodyhit1.wav", "player/bodyhit2.wav", "player/bodyhit3.wav",
		"player/bodyhit4.wav", "player/die.wav", "player/die1.wav", "player/die2.wav",
		"player/die3.wav", "player/pain1.wav", "player/pain2.wav", "player/pain3.wav",
		"player/pain4.wav", "misc/hacking.wav", "misc/hacked.wav", "misc/failed.wav",
		"misc/successful.wav", "misc/gogogo.wav", "debris/ric1.wav", "debris/ric2.wav",
		"debris/ric3.wav", "debris/ric4.wav", "debris/ric5.wav", "debris/cement.wav",
		"debris/cement1.wav", "debris/cement2.wav", "debris/cement3.wav", "debris/cement4.wav",
		"debris/cement5.wav", "debris/cement6.wav", "debris/gravel1.wav", "debris/gravel2.wav",
		"debris/gravel3.wav", "debris/gravel4.wav", "debris/metal1.wav", "debris/metal2.wav",
		"debris/metal3.wav", "debris/metal4.wav", "debris/metal5.wav", "weapons/shell.wav",
		"weapons/sshell.wav"};
	for (unsigned i = 0; i < sizeof(fixed1) / sizeof(fixed1[0]); i++)
		PRECACHE_SOUND((char *)fixed1[i]);

	// 2. the weapon-table loop (models + fire sound per weapon)
	for (int i = 0; i < 0x26; i++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)i);
		if (wi->pszViewModel && wi->pszViewModel[0])
			PRECACHE_MODEL((char *)wi->pszViewModel);
		if (wi->pszPlayerModel && wi->pszPlayerModel[0])
			PRECACHE_MODEL((char *)wi->pszPlayerModel);
		if (wi->pszWorldModel && wi->pszWorldModel[0])
			PRECACHE_MODEL((char *)wi->pszWorldModel);
		if (wi->pszFireSound && wi->pszFireSound[0])
			PRECACHE_SOUND((char *)wi->pszFireSound);
	}

	// 3. the post-loop fixed set
	static const char *const fixed2[] = {
		"weapons/pistol-empty.wav", "weapons/insert-shell.wav", "weapons/goslow.wav",
		"weapons/whizz1.wav", "weapons/whizz2.wav", "weapons/whizz3.wav", "weapons/whizz4.wav",
		"weapons/whizz5.wav", "player/closecombat.wav", "player/kungfuhit.wav",
		"weapons/weapondrop.wav", "weapons/items.wav", "weapons/draw.wav", "weapons/switch.wav",
		"items/9mmclip1.wav", "weapons/generic_reload.wav", "explo/explode.wav",
		"explo/explode1.wav", "explo/explode2.wav", "explo/explode3.wav", "explo/explode4.wav",
		"weapons/knife/knife_miss.wav", "weapons/knife/knife_hit.wav",
		"weapons/knife/knife_hitbody.wav", "weapons/katana/hit-body.wav",
		"weapons/katana/miss1.wav", "weapons/katana/miss.wav", "weapons/katana/hit.wav",
		"weapons/katana/hit1.wav"};
	for (unsigned i = 0; i < sizeof(fixed2) / sizeof(fixed2[0]); i++)
		PRECACHE_SOUND((char *)fixed2[i]);
	PRECACHE_MODEL("models/generic_clip.mdl");
	PRECACHE_MODEL("models/ammopack.mdl");
	PRECACHE_MODEL("models/bullet.mdl");
	PRECACHE_MODEL("models/long_bullet.mdl");
	PRECACHE_MODEL("models/pellet.mdl");
	UTIL_PrecacheOther("m61_grenade"); // pulls the grenade's own Precache (grenbounce*.wav) into the window
	UTIL_PrecacheOther("shrapnel");
	UTIL_PrecacheOther("ts_flag");
}

// @0x8fb4c RemoveEntityFromSlowedArray(ent) — scrub a dying entity out of EVERY player's
// packed slowed array (+0x208, count +0x288): NULL each found slot (all occurrences; holes
// kept, the count never shrinks). Every slowed-projectile removal path (flag / worldgun x3 /
// knife x2 / grenade / shrapnel) calls this right before UTIL_Remove — the array holds RAW
// pointers, so a missed scrub dangles.
void RemoveEntityFromSlowedArray(CBaseEntity *pEnt)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *p = UTIL_PlayerByIndex(i);
		if (!p || !p->IsPlayer())
			continue;
		CBasePlayer *pl = (CBasePlayer *)p;
		if (pl->m_iSlowedEntCount <= 0)
			continue;
		for (int k = 0; k < pl->m_iSlowedEntCount; k++)
		{
			int found = UTIL_EntInArray(pl->m_pSlowedEnts, pl->m_iSlowedEntCount, pEnt);
			if (found >= 0)
				pl->m_pSlowedEnts[found] = NULL;
		}
	}
}

// @0x8f94c TSPlayerDisconnect(player) — the leaver teardown (called from ClientDisconnected):
// drop a carried flag; CLEAR THE PLAYER'S VOTE BLOCK (rules+0x684+idx*0x48 — a rejoiner into
// the same slot no longer inherits the leaver's votes: the vote-rejoin bug); park the edict
// (non-solid, DEAD_DEAD, no damage, FL_FAKECLIENT off, FL_DORMANT on, EF_NODRAW, frags/health
// zeroed, respawn gate -4); clear the The-One flag (the leaver-stays-the-One leak); then
// CallCheckDeads — a disconnect can END a team round (the last enemy left).
void TSPlayerDisconnect(CBasePlayer *pPlayer)
{
	pPlayer->DropFlag(false);
	CTSGameRules *pRules = dynamic_cast<CTSGameRules *>(g_pGameRules);
	const int idx = ENTINDEX(pPlayer->edict());
	if (pRules && idx >= 0 && idx < 32)
	{
		memset(pRules->m_voteMap[idx], 0, sizeof(pRules->m_voteMap[idx]));
		pRules->m_voteFF[idx] = 0;
		pRules->m_voteTP[idx] = 0;
	}
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->deadflag = DEAD_DEAD;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->flags &= ~FL_FAKECLIENT;
	pPlayer->pev->flags |= FL_DORMANT;
	// player+0x160 = 0 (unnamed transient)
	pPlayer->pev->frags = 0;
	// player+0xb30 = 0 / +0xb48 byte = 0 / +0x9b8 = 0 / +0x534 = -4.0f (unnamed transients)
	pPlayer->m_flTSRespawnTime = -4.0f; // 0x770
	pPlayer->pev->effects = EF_NODRAW;
	pPlayer->pev->health = 0;
	pPlayer->m_bTheOne = 0; // 0x7b9
	if (pRules)
		pRules->CallCheckDeads();
}

// @0x8fae0 PlayerIsCheating — bots never cheat; a human cheats iff their cl_lw != "1"
// (the forced-client-side-weapons check). Callers: ClientUserInfoChanged (which then forces
// cl_lw "1") and TSInit (result discarded — the original's quirk).
bool PlayerIsCheating(CBasePlayer *pPlayer, char *infobuffer)
{
	if (pPlayer->pev->flags & FL_FAKECLIENT)
		return false;
	const char *lw = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_lw");
	return !lw || strcmp(lw, "1") != 0;
}

// @0x8f238 TSCountPlayers — players on `team` (case-insensitive TeamID match; NULL/empty team =
// everyone); onlyActive counts the alive PLUS the dead-but-respawning (+0x770 >= 0).
int TSCountPlayers(const char *team, bool onlyActive)
{
	int n = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!p)
			continue;
		if (team && team[0] && (!p->TeamID() || strcasecmp(team, p->TeamID()) != 0))
			continue;
		if (onlyActive && !p->IsAlive() && p->m_flTSRespawnTime < 0)
			continue;
		n++;
	}
	return n;
}

// ---- The CustomWP pipeline (dossier-freefuncs F2b / dossier-gmsg U5; analysis_ff_util §4-7) ----

// "CustomWP" (-1) — ONE WRITE_STRING: "%i %f %f %f %f %i" = id accuracy viewkick
// accuracykick damage cost. The ASCII line IS the wire format (drop-in critical).
int gmsgTSCustomWeaponParams = 0;

// @0x8eb98 SendWeaponParams — one weapon's tunables to one client (MSG_ONE) or everyone (MSG_ALL).
void SendWeaponParams(tsweapon_t id, CBaseEntity *pOnly)
{
	if ((unsigned)id >= 0x26)
		return;
	const s_weapon_info *wi = get_weapon_info(id);
	if (!wi->pszName || !wi->pszName[0])
		return;
	char buf[64];
	sprintf(buf, "%i %f %f %f %f %i", id, wi->flSpread, wi->flViewKick, wi->flRecoil,
		wi->flDamage, wi->iPrice);
	buf[63] = 0;
	if (pOnly)
		MESSAGE_BEGIN(MSG_ONE, gmsgTSCustomWeaponParams, NULL, pOnly->edict());
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgTSCustomWeaponParams);
	WRITE_STRING(buf);
	MESSAGE_END();
}

// @0x8eb14 InformClientsOnWeaponsParams — the "custom weapons" banner + the full 38-slot sync.
void InformClientsOnWeaponsParams(CBaseEntity *pOnly)
{
	if (pOnly)
		TSHudMessageOne(45, 195, 45, 8, 100, "This server is using custom weapons.", pOnly->pev);
	else
		TSHudMessageAll(45, 195, 45, 8, 100, "This server is using custom weapons.");
	for (int i = 0; i < 0x26; i++)
		SendWeaponParams((tsweapon_t)i, pOnly);
}

// ⚠ SPARTA STAGE ORDER: the tunable quartet here (spread/viewkick/recoil/damage) is the
// CustomWP RUNTIME THIRD STAGE — it overwrites Sparta(), which itself overwrote the Write_
// init ([[sparta-overrides-write-weapon]]). Stage order: Write_ (dead for these four) ->
// Sparta() (the shipped live values) -> adjust_weapon_value/AdjustSpecificWeapon (server
// customization, broadcast via CustomWP so clients re-sync). When "verifying" a combat value,
// know WHICH stage you're reading: a Write_ disasm read is two stages stale.
// @0x8ea3c AdjustSpecificWeapon — set one weapon's tunable quintet (no clamps), broadcast it,
// announce, and flag the rules custom. DEAD in the shipped Linux .so (the console path was
// compiled out) — kept callable for parity.
void AdjustSpecificWeapon(tsweapon_t id, float acc, float vkick, float akick, float dmg, int cost)
{
	if ((unsigned)id >= 0x26)
		return;
	s_weapon_info *wi = (s_weapon_info *)get_weapon_info(id);
	wi->flSpread = acc;
	wi->flViewKick = vkick;
	wi->flRecoil = akick;
	wi->flDamage = dmg;
	wi->iPrice = cost;
	SendWeaponParams(id, NULL);
	UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("%s behaviour changed\n", wi->pszName));
	if (TSGameRules())
		TSGameRules()->m_bWeaponParamsCustom = 1;
}

// @0x8eca4 adjust_weapon_value — load + apply a weapons config file from the game dir.
// ⚠ SPARTA STAGE ORDER: this is the CustomWP third stage over Sparta() — see the banner above.
// Exact behaviors kept: "//" ANYWHERE = comment; armor_absorb/health_absorb lines (clamped
// >= 0) land in the rules; a weapon line with fewer than 6 fields ABORTS with the original
// console string but still returns 1; accuracy floors at 0.001 (the only clamp); EOF
// announces + sets the custom flag. Returns 0 only when the file can't open.
int adjust_weapon_value(char *pszFile)
{
	char dir[64];
	GET_GAME_DIR(dir);
	char path[192];
	sprintf(path, "%s%s%s", dir, "/", pszFile);
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return 0;

	char raw[1024];
	while (fgets(raw, sizeof(raw), fp))
	{
		char line[256];
		strncpy(line, raw, sizeof(line) - 1);
		line[sizeof(line) - 1] = 0;
		int len = (int)strlen(line);
		while (len > 0 && isspace(line[len - 1]))
			line[--len] = 0;
		if (len == 0)
			continue;
		if (strstr(line, "//"))
			continue; // comment anywhere kills the line
		float v;
		if (strstr(line, "armor_absorb"))
		{
			if (sscanf(strstr(line, "armor_absorb") + 12, " %f ", &v) == 1)
			{
				if (v < 0)
					v = 0;
				if (TSGameRules())
					TSGameRules()->m_flArmorAbsorb = v; // rules+0x588c
			}
			continue;
		}
		if (strstr(line, "health_absorb"))
		{
			if (sscanf(strstr(line, "health_absorb") + 13, " %f ", &v) == 1)
			{
				if (v < 0)
					v = 0;
				if (TSGameRules())
					TSGameRules()->m_flHealthAbsorb = v; // rules+0x5888
			}
			continue;
		}
		int id, cost;
		float acc, vkick, akick, dmg;
		if (sscanf(line, " %i %f %f %f %f %i ", &id, &acc, &vkick, &akick, &dmg, &cost) < 6)
		{
			ALERT(at_console, "You need to run saveweapons again.\n");
			fclose(fp);
			return 1; // mid-file abort still reports handled (the original's literal behavior)
		}
		if ((unsigned)id < 0x26)
		{
			s_weapon_info *wi = (s_weapon_info *)get_weapon_info((tsweapon_t)id);
			if (wi->pszName && wi->pszName[0])
			{
				wi->flSpread = (acc < 0.001f) ? 0.001f : acc; // the only clamp
				wi->flViewKick = vkick;
				wi->flRecoil = akick;
				wi->flDamage = dmg;
				wi->iPrice = cost;
			}
		}
	}
	fclose(fp);
	UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("weapons behaviour changed by %s\n", pszFile));
	if (TSGameRules())
		TSGameRules()->m_bWeaponParamsCustom = 1;
	return 1;
}

// @0x8f044 save_weapon_value — write the weapons config file (the exact original header;
// footer saves armor_absorb ONLY — health_absorb is parseable but never written, asymmetry kept).
int save_weapon_value(char *pszFile)
{
	char dir[64];
	GET_GAME_DIR(dir);
	char path[192];
	sprintf(path, "%s%s%s", dir, "/", pszFile);
	FILE *fp = fopen(path, "w");
	if (!fp)
		return 0;

	fputs(
		"//===========================================\n"
		"//values are : |weapon id|accuracy|view kick|accuracy kick|damage|cost|\n\n"
		"//IMPORTANT:\n"
		"//-dont change weapon id!\n"
		"//-accuracy cant go under 0.001\n"
		"//-if you put an accuracy kick of 0 accuracy will not be scaled\n\n"
		"//If you want use different weapon settings use the 'customweapons' cvar.\n"
		"//Example: 'customweapons weapons_custom.txt'\n"
		"//===========================================\n\n",
		fp);
	for (int i = 0; i < 0x26; i++)
	{
		const s_weapon_info *wi = get_weapon_info((tsweapon_t)i);
		if (!wi->pszName || !wi->pszName[0])
			continue;
		fprintf(fp, "//%s\n%i %f %f %f %f %i\n\n", wi->pszName, i, wi->flSpread,
			wi->flViewKick, wi->flRecoil, wi->flDamage, wi->iPrice);
	}
	// footer: armor_absorb ONLY (health_absorb is parseable by the loader but never saved —
	// the original's asymmetry, kept). .rodata format "armor_absorb %f".
	fprintf(fp, "armor_absorb %f\n", TSGameRules() ? TSGameRules()->m_flArmorAbsorb : 0.0f);
	fclose(fp);
	return 1;
}
