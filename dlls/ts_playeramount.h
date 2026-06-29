/***
*  ts_playeramount.h — CTSPlayerAmount (player-count relay) declaration so the engine
*  ClientConnect/ClientDisconnect interface hooks (client.cpp) can drive its per-event
*  counter methods, exactly as the original TS_entities.cpp does.
***/
#ifndef TS_PLAYERAMOUNT_H
#define TS_PLAYERAMOUNT_H

// A map-placed point relay (ts_playeramount): each engine client connect/disconnect bumps a
// per-entity counter; when it reaches `clients` it fires connect/disconnecttarget. The original
// drives this off CTSPlayerAmount::ClientConnect/ClientDisconnect (NOT a poll).
class CTSPlayerAmount : public CBaseDelay
{
public:
	bool KeyValue(KeyValueData *pkvd) override;
	void Spawn() override;
	void ClientConnect();      // @0x72aec — count up, fire connecttarget on == clients
	void ClientDisconnect();   // @0x72c08 — count down, fire disconnecttarget on == clients

	int      m_iCount = 0;              // +0x6c — running connect/disconnect counter (Spawn=0)
	int      m_iClients = 0;            // +0x70 — fire when the counter reaches this
	string_t m_iszConnectTarget = 0;    // +0x74 — fired when the count rises to m_iClients
	string_t m_iszDisconnectTarget = 0; // +0xd8 — fired when the count falls to m_iClients
};

#endif // TS_PLAYERAMOUNT_H
