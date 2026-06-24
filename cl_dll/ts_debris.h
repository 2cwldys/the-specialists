/***
*  ts_debris.h — The Specialists glass-shatter sprite shower (client side).
*
*  Faithful to client.dll FUN_10047cb0 (the debris spawner) + its MsgFunc @0x1004becc.
*  GLASS breakables (CBreakable::Die material 0, @0x752e0) send the custom `gmsgTSCreateDebris`
*  message instead of TE_BREAKMODEL; the client lays a 9x9 = 81-fragment grid of
*  `sprites/glassfrag.spr` across the pane FACE, each fragment flying in the shatter direction
*  (= the bullet/attack direction, g_vecAttackDir) with spread + gravity, living 2-5s. Other
*  materials (wood/metal/...) keep TE_BREAKMODEL (the gib MODEL) — see reference/BULLETS.md.
***/
#if !defined(TS_DEBRIS_H)
#define TS_DEBRIS_H

class CHudTSDebris : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool MsgFunc_TSCreateDebris(const char* pszName, int iSize, void* pbuf);
};

#endif // TS_DEBRIS_H
