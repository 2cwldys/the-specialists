# Engine ↔ mod interface — the complete contract + status

The mission's compat target is the **external interface** (CLAUDE.md): every way the
engine talks to or with the mod. This tracks ALL of it for completeness. Four surfaces:
(1) engine → server callbacks, (2) engine → client callbacks, (3) the by-name protocol
(messages/events/cvars/commands — in `API_SURFACE.md`/`HUD_MESSAGES.md`), (4) mod →
engine calls (enginefuncs).

Status: ✅ TS-faithful · 🟡 partial · ⬜ stock-inherited (works, no TS-specific behaviour
needed) · ❌ TS behaviour missing/stub.

---

## 1. Engine → SERVER callbacks — `DLL_FUNCTIONS` (filled in `dlls/cbase.cpp`)

Most are stock SDK plumbing (⬜) that work as inherited. TS-relevant ones:

| callback | status | notes |
|---|:--:|---|
| `pfnGameInit` | ✅ | registers TS cvars (cont.54) |
| `pfnSpawn` | ✅ | all TS `LINK_ENTITY_TO_CLASS` factories (Track A) |
| `pfnKeyValue` | ✅ | TS entity keyvalues (FGD-driven) |
| `pfnClientConnect`/`pfnClientPutInServer` | ✅ | TS spawn: weapon_tsgun + BuyArmament (cont.49) |
| `pfnClientCommand` | ✅ | TS console cmds (tsbuy/+alt2/…); see API_SURFACE §6c |
| `pfnPlayerPreThink` | 🟡 | UpdatePowerUpState ✅; full TS prethink TBD |
| `pfnPlayerPostThink` | ❌ | **MISSING**: CTSAnimation::Animate + CTSStunt::GoRoll + CheckPowerups (VERIFICATION §C) |
| `pfnUpdateClientData` | ✅ | TS HUD messages via TSUpdateClientData (cont.51) |
| `pfnPM_Move` (player movement) | ❌ | **the visible slow-motion time-dilation lives here** — pmove must scale frametime by the slow field; NOT wired (the long-standing SLICE) |
| `pfnPM_Init`/`pfnPM_FindTextureType` | ⬜ | stock pmove |
| `pfnThink`/`pfnUse`/`pfnTouch`/`pfnBlocked` | ✅ | dispatch to TS entity methods |
| `pfnSave`/`pfnRestore`/`pfnSaveWriteFields`/`pfnSaveReadFields`/`pfnSaveGlobalState`/`pfnRestoreGlobalState`/`pfnResetGlobalState` | 🟡 | stock save/restore; many TS classes have no DEFINE_FIELD save data yet |
| `pfnServerActivate`/`pfnServerDeactivate`/`pfnStartFrame` | ⬜ | stock; gamerules Think runs via the world |
| `pfnClientUserInfoChanged` | 🟡 | stock; TS team/model handling partial |
| `pfnGetGameDescription` | 🟡 | returns the SDK desc, not "The Specialists" — TODO |
| `pfnClientKill`/`pfnClientDisconnect` | ⬜/🟡 | TSPlayerDisconnect cleanup TBD |
| `pfnSetupVisibility`/`pfnAddToFullPack`/`pfnCreateBaseline`/`pfnRegisterEncoders`/`pfnGetWeaponData`/`pfnCmdStart`/`pfnCmdEnd`/`pfnUpdateClientData` | ⬜ | stock netcode/prediction plumbing |
| `pfnConnectionlessPacket`/`pfnInconsistentFile`/`pfnAllowLagCompensation`/`pfnGetHullBounds`/`pfnCreateInstancedBaselines`/`pfnParmsNewLevel`/`pfnParmsChangeLevel`/`pfnPlayerCustomization`/`pfnSpectatorConnect`/`pfnSpectatorDisconnect`/`pfnSpectatorThink` | ⬜ | stock |
| `NEW_DLL_FUNCTIONS`: `pfnOnFreeEntPrivateData`/`pfnGameShutdown`/`pfnShouldCollide`/`pfnCvarValue*` | ⬜ | stock |

## 2. Engine → CLIENT callbacks — cldll exports (`cl_dll/cdll_int.cpp`)

| export | status | notes |
|---|:--:|---|
| `Initialize`/`HUD_Init` | ✅ | inits TS HUD elems + cvars + commands + events |
| `HUD_VidInit` | ✅ | TS HUD + weapon-bar sprite load |
| `HUD_Redraw` | ✅ | draws the TS HUD (3 regions) |
| `HUD_UpdateClientData` | 🟡 | stock; reads usercmd — TS slow-motion scaling NOT applied here |
| `HUD_PlayerMove`/`HUD_PlayerMoveInit`/`HUD_PlayerMoveTexture` | ❌ | **client pmove — the slow-motion frametime scaling belongs here too (mirror of pfnPM_Move); NOT wired** |
| `CL_CreateMove` | 🟡 | mouselook forced on (cont.49); TS view/bob TBD |
| `CL_IsThirdPerson`/`CAM_Think`/`CL_CameraOffset` | ✅ | thirdperson MP gate removed (cont.60) |
| `HUD_PostRunCmd` | 🟡 | stock weapon prediction; **CTSGun NOT client-predicted** (true predicted-fire needs this) |
| `HUD_Frame`/`HUD_AddEntity`/`HUD_CreateEntities`/`HUD_TempEntUpdate`/`HUD_StudioEvent` | ⬜ | stock |
| `HUD_DrawNormalTriangles`/`HUD_DrawTransparentTriangles` | ⬜ | stock; TS particle system would hook here |
| `HUD_Key_Event`/`IN_*`(mouse/accumulate) | 🟡 | stock + always-mouselook |
| `HUD_GetStudioModelInterface` | ⬜ | stock studio renderer (the warning re HUD_GetSoundInterface is benign) |
| `Game_HookEvents` | ✅ | hooks events/fire.sc → EV_TSFire (cont.56) + stock events |
| `HUD_VoiceStatus`/`HUD_DirectorMessage`/`HUD_ProcessPlayerState`/`HUD_TxferLocalOverrides`/`HUD_TxferPredictionData`/`HUD_GetUserEntity`/`HUD_GetHullBounds`/`HUD_ConnectionlessPacket`/`HUD_Reset`/`HUD_Shutdown`/`HUD_ChatInputPosition`/`V_CalcRefdef`/`Demo_*`/`KB_Find`/`CL_InitClient`/`CL_Load/UnloadParticleMan` | ⬜ | stock |

## 3. By-name protocol
User messages → `HUD_MESSAGES.md`. Cvars/commands/events + impl status → `API_SURFACE.md` (§1–6c).

## 4. Mod → ENGINE calls (enginefuncs / cl_enginefuncs)
Used as needed; the TS-relevant ones in play: `pfnRegUserMsg`, `MESSAGE_BEGIN/WRITE_*`, `PRECACHE_*`,
`PLAYBACK_EVENT`, `EMIT_SOUND`, `pfnCVarRegister`, `pfnAddServerCommand`; client `pfnHookUserMsg`,
`pfnHookEvent`, `pfnAddCommand`, `RegisterVariable`, `SPR_*`, `pfnFillRGBA`, `pfnDrawCharacter`,
`pEventAPI->*`. (Not individually status-tracked — they're leaf calls, correct by use.)

---

## The notable TS-specific GAPS (the real remaining interface work)
1. **`pfnPM_Move` + `HUD_PlayerMove`** — the **visible slow-motion time-dilation** (scale pmove frametime
   by the player's slow field). The server slow state is complete (GoSlow/InSlow/GlobalSlowMotion2,
   slowmatch); nothing reads it in movement. This is the biggest missing interface behaviour.
2. **`pfnPlayerPostThink`** — `CTSAnimation` (gait/anim) + `CTSStunt` (dives/rolls/wall-flips).
3. **`HUD_PostRunCmd`** — client-side CTSGun weapon prediction (true predicted fire/alt-fire).
4. **`pfnGetGameDescription`** — return "The Specialists".
5. Save/Restore field data for the TS entity/player state.
