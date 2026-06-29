/***
*  ts_briefcase.h — CTSBriefcase (original TS_objectives.cpp), the CTF base + flag registry.
*
*  CTSBriefcase is the map-placed (`@SolidClass`, brush) CTF objective — one per team. It OWNS
*  the flag: it stamps a template `s_flag_status` (with an EHANDLE back to itself) and seeds its
*  team's loose CTSFlag at its centre (FlagFall). It is also the capture zone: a player carrying
*  the enemy flag who reaches THEIR OWN briefcase captures it (CaptureTouch). The flag never walks
*  itself home — on return/capture it reschedules the SOURCE briefcase's FlagFall via the EHANDLE.
***/
#ifndef TS_BRIEFCASE_H
#define TS_BRIEFCASE_H

class CTSBriefcase : public CBaseEntity
{
public:
	CTSBriefcase();                                // @0x7d788 — assign a cycling flag id (1..15)
	bool KeyValue(KeyValueData *pkvd) override;   // @0x7d7f4 — ownerteam, contents
	void Spawn() override;                         // @0x7d88c — brush trigger + stamp the template status
	void EXPORT FlagFall();                        // @0x7da00 — seed this team's flag at the base
	void EXPORT WaitThink();                       // @0x7db14 — idle keepalive
	bool CanInteract(CBasePlayer *pPlayer);        // @0x7db48 — owner-team gate
	void EXPORT CaptureTouch(CBaseEntity *pOther); // @0x7db9c — capture a carrier of the enemy flag

	// @0x1779a0 — static id source: each briefcase/flag gets a cycling id 1..15.
	static int s_nextFlagID;

	bool          m_bFlagOut = false;  // +0x64 — its flag entity is currently spawned/loose
	int           m_iOwnerTeam = -1;   // +0x68 — owning team, 0-based (ownerteam KV − 1); -1 = unset
	char          m_szContents[32] = {}; // +0x6c — "contents" description (strncpy, ≤31)
	int           m_iFlagID = 0;       // +0x8c — id assigned from s_nextFlagID
	s_flag_status m_flagStatus;        // +0x90 — the template stamped onto every flag this seeds
};

#endif // TS_BRIEFCASE_H
