/***
*  ts_debris.h — the TSFrags message (client.dll 0x1004bd50): kind 0 the glass shower (FUN_10047cb0,
*  sent by CBreakable::Die for glass, 0x752e0), 1 a grenade explosion (FUN_100481b0), 2 a metal hit.
***/
#if !defined(TS_DEBRIS_H)
#define TS_DEBRIS_H

class CHudTSDebris : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool MsgFunc_TSFrags(const char* pszName, int iSize, void* pbuf);
};

#endif // TS_DEBRIS_H
