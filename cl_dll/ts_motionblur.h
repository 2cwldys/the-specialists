/***
*  ts_motionblur.h — The Specialists slow-motion GHOST TRAILS (the TSMBlur user message).
*
*  RE (reference/dossiers/cont94/client_message_hooks.md §1): the original hooks TSMBlur on
*  the TS master hud element (handler FUN_1004bf90 → arm FUN_10029680 on the 32-entry blur
*  array DAT_100dff48), and spawns the ghosts per frame from HUD_CreateEntities
*  (FUN_10029780) as CL_TentEntAllocCustom tempents that carry the player's entity_state_t
*  with rendercolor.b=1 as THE GHOST MARKER (rendercolor.r = owner entindex), fading via
*  the callback FUN_10029710 (255 alpha/sec ÷ timescale). The original filename is
*  unrecoverable; this is a plain module (no CHudBase element).
***/
#pragma once

void TS_MotionBlur_Init();      // hook the TSMBlur user message (call from CHud::Init)
void TS_UpdateMotionBlur();     // per-frame ghost spawner (call from HUD_CreateEntities)
