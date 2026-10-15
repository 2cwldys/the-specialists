# Deviations from TS 3.0

reTS matches the original's external interface (see CLAUDE.md). This file lists where it does not match
the original: first the additions made on purpose, then the differences the build forces. coverage.py
reports each added cvar or export as EXTRA; this file is why that is expected.

## Deliberate

Each of these leaves the original's behaviour as the default unless it says otherwise.

### mp_teammodels_force (server cvar, default 1)

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

### ReTS_GetInterface (server export)

A versioned struct of get/set functions for the player fields AMX Mod X's TS module touches (cash, slow
motion, kevlar, free slots and the rest), so it does not depend on member offsets. reTSX, the TS module in
goldsrc-net's AMX Mod X fork, uses it whenever the game library exports it. Not in the original; it exists
because of the layout differences below. Detail: CLAUDE.md §4a, `dlls/rets_api.h`.

## Forced by the build

None of these were chosen. They follow from building TS's code with a modern compiler on Half-Life
Updated, and they break anything that reaches into the game library by address instead of by name.

### Member offsets

reTS's objects do not have TS 3.0's memory layout, so a field is not at the offset the original has it at:

- gcc 2.96, which built TS 3.0, put the vtable pointer at `+0x60` in each object; every modern compiler
  puts it at `+0`, which moves every field before it.
- Half-Life Updated's base classes have members the 2007 SDK did not.
- On amd64 and arm64 pointers are 8 bytes, which moves every field after the first pointer.

Offsets are moved toward the original's where feasible, and recovered offsets stay in the code as
comments, but an exact match is not reachable. `scripts/coverage/offsets.py` maps original offsets to
reTS's, per architecture.

What it breaks: an AMX Mod X plugin or module that reads or writes TS 3.0 offsets directly
(`get_pdata`/`set_pdata`, stock TSX) reads the wrong fields on reTS. reTSX avoids this through
`ReTS_GetInterface`; a plugin that pokes offsets itself has no fix short of the layout matching.

### Vtable slots

reTS keeps the original's virtuals in the original's order, plus a few that every Half-Life Updated mod
has (listed in CLAUDE.md: the virtual destructor, `MyTogglePointer`, the HL25 sentence virtuals on
`CBaseToggle`, the client-prediction virtuals on `CBasePlayerItem`, `DoRetireWeapon`). Those shift the
slot numbers after them, so Ham Sandwich's stock TS gamedata hooks the wrong functions. reTS has its own,
`gamedata/common.games/virtual.games/ts/offsets-rets.txt` in the AMX Mod X fork, written by
`scripts/coverage/hamdata.py` and selected by the `ReTS_GetInterface` export.

### State kept differently

A few values are the same as the original's to a plugin calling the game, but are not stored the way the
original stores them, so even a matched offset would not hold them:

- Free slots: reTS derives the count from what the player carries; setting it stores the difference in
  `CBasePlayer::m_iFreeSlotsAdjust`.
- Slow motion: the original keeps a registry of slowed players in the game rules; reTS re-applies slow
  motion each frame and keeps no such array (`CTSGameRules::TogglePlayerFromSlow`).
