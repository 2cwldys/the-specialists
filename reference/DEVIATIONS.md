# Deliberate deviations from TS 3.0

reTS matches the original's external interface (see CLAUDE.md). The entries below are additions or
changes made on purpose, on request, that the original does not have. Each one leaves the original's
behaviour as the default unless it says otherwise. coverage.py reports each added cvar or export as
EXTRA; this file is why that is expected.

## mp_teammodels_force (server cvar, default 1)

TS 3.0's `CHalfLifeTeamplay::ClientUserInfoChanged` (ts_i386.so 0xd4d18) compares the client's `model`
key to the team name, and on any mismatch writes a random model from the team's `mp_teammodels` list
(`CTSGameRules::GetPlayerModel`, 0x69a08). Team names never match a model, so any userinfo change (a
name change, a rate change) re-rolls the player's model, and a plugin that sets custom models has to set
it again after each one. Every write goes out to every client.

- `1`: the original behaviour.
- `0`: a model from the player's own team list is kept, and the pick remembered
  (`CBasePlayer::m_iPlayerModel`). Any other model, the other teams' included, is put back to the team
  model the player already had instead of a new random one. A team change (`ChangePlayerTeam`) still
  stamps a random one from the new team's list, as the original does.

Players stay restricted to their own team's set either way. The set keeps the original's limits:
three models per team, fifteen characters each, read when the map loads. A plugin can set a model
outside the set, but the next userinfo change from that player puts it back.

Code: `dlls/game.cpp` (the cvar), `dlls/teamplay_gamerules.cpp` (the gate).
Requested by a server operator running model plugins under teamplay, 2026-10-15.

## ReTS_GetInterface (server export)

A versioned struct of player-field accessors for AMX Mod X's TSX module, so it does not depend on member
offsets. Not in the original. Detail: CLAUDE.md §4a, `dlls/rets_api.h`.
