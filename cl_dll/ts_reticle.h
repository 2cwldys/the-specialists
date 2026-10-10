/***
*  ts_reticle.h — the crosshair, the dive arc, the laser dots and the bomb wires (client.dll's weapon
*  bar draw tail, 0x1004d3f0 onward). See ts_reticle.cpp.
***/
#ifndef TS_RETICLE_H
#define TS_RETICLE_H

class CHudTSReticle : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Think() override; // the stunt time runs down (0x1004ce36)
	bool MsgFunc_NextStunt(const char* pszName, int iSize, void* pbuf); // stunt time byte (x0.1 s)

private:
	void DrawLaserDots();                        // 0x1004e6f0
	void DrawWires(const unsigned char* wires);  // 0x1004d550
	void DrawArc(int cx, int cy);                // 0x1004ef50
	void DrawCrosshair();                        // 0x1004e9f0

	HSPRITE m_hCrossE = 0, m_hCross2 = 0, m_hCrossT = 0; // cross_e, cross2, cross_t (64 x 64)
	HSPRITE m_hDive[3] = {};                              // divestept, divestepc, divestepb (40 x 80)
	HSPRITE m_hWire[3] = {}, m_hWireCut[3] = {};          // r/g/bwire, r/g/bwirecutted

	float m_flStuntTime = 0; // weapon bar +0x34: NextStunt's time, run down by Think
	int m_iLaserRange = 0;   // weapon bar +0x18: the laser range number
};

#endif // TS_RETICLE_H
