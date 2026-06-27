/***
*  ts_reticle.h — The Specialists DIVE RETICLE (RE reference: client.dll FUN_1004cb50 init +
*  FUN_1003254e draw). The crosshair shows DIVE READINESS: a left-side arc of three "dive step"
*  segments (divestept/c/b = top/center/bottom) colored by loadout weight — light = blue, fully
*  heavy = red, 0..3 lit by how much capacity remains — plus a dot/highlight tracking aim pitch
*  (which dive height tier your current aim would give). Sprite handles (RE: the static reticle
*  object @ client.dll+0xd0ca0): cross_e/cross2/cross_t (64x64 crosshair) + divestept/c/b (40x80
*  arc segments). Drawn additive. Weight derived client-side from the networked maxspeed (the
*  cont.69z slot formula); pitch from the view angles. See reference/STUNT.md (dive tiers).
***/
#ifndef TS_RETICLE_H
#define TS_RETICLE_H

class CHudTSReticle : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;

private:
	HSPRITE m_hCrossE = 0, m_hCross2 = 0, m_hCrossT = 0; // 64x64 center crosshair pieces
	HSPRITE m_hDiveT = 0, m_hDiveC = 0, m_hDiveB = 0;    // 40x80 dive-step arc (top/center/bottom)
};

#endif // TS_RETICLE_H
