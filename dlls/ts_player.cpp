/***
*  ts_player.cpp — RECONSTRUCTED TS player extensions (operate on CBasePlayer).
*  Functional/source-faithful (see CLAUDE.md).
***/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "ts_gamerules.h" // TS_PWUP_* bit constants
#include "ts_player.h"

// @ SlowMotion__FP11CBasePlayer
bool SlowMotion(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_SLOWMO) != 0;
}

// @ InfiniteClips__FP11CBasePlayer
bool InfiniteClips(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_INFCLIP) != 0;
}

// @ SlowPause__FP11CBasePlayer
bool SlowPause(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_SLOWPAUSE) != 0;
}

// @ DoubleFireRate__FP11CBasePlayer
bool DoubleFireRate(CBasePlayer *pPlayer)
{
	return (pPlayer->m_iPowerups & TS_PWUP_DBLFIRE) != 0;
}

// @ KungFu__FP11CBasePlayer
int KungFu(CBasePlayer *pPlayer)
{
	return pPlayer->m_iKungFu;
}
