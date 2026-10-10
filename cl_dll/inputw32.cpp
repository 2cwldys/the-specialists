//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "../public/keydefs.h"
#include "view.h"
#include "Exports.h"

#include "PlatformHeaders.h"

#include <cmath>

// Mouse-look delta source is per-target: Xash (wasm + native) PUSHES deltas (handled by the
// s_pushInputSeen flag below); native GoldSrc reads the OS itself, split by platform —
//   * Windows -> Win32 GetCursorPos/SetCursorPos, exactly like the original ts client.dll (no SDL)
//   * Linux   -> SDL_GetRelativeMouseState, exactly like valve's client.so (SDL is mandatory there)
// GoldSrc is 32-bit, so SDL is confined to i386 Linux; Windows (no SDL2.lib, on purpose), amd64
// and wasm never reference it. This is the platform split upstream never guarded (SDL was
// unconditional, which is why the Windows build pulled in SDL2).
#if defined(__i386__) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define TS_INPUT_SDL 1
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#endif

void IN_ResetMouse();
bool TS_VGUIMenuUp(); // vgui_TeamFortressViewport.cpp

#define MOUSE_BUTTON_COUNT 5

// Set this to 1 to show mouse cursor.  Experimental
bool g_iVisibleMouse = false;

extern cl_enginefunc_t gEngfuncs;
extern bool iMouseInUse;

extern kbutton_t in_strafe;
extern kbutton_t in_mlook;
extern kbutton_t in_speed;
extern kbutton_t in_jlook;

extern cvar_t* m_pitch;
extern cvar_t* m_yaw;
extern cvar_t* m_forward;
extern cvar_t* m_side;

extern cvar_t* lookstrafe;
extern cvar_t* lookspring;
extern cvar_t* cl_yawspeed;
extern cvar_t* cl_sidespeed;
extern cvar_t* cl_forwardspeed;
extern cvar_t* cl_pitchspeed;
extern cvar_t* cl_movespeedkey;


static cvar_t* m_rawinput = nullptr;

static bool IN_UseRawInput()
{
	// m_rawinput 1 is the engine-push input path (the engine reads the mouse and
	// pushes deltas). On Linux/xash it is always the push path; on WIN32/GoldSrc it
	// falls back to the classic GetMousePosition path when m_rawinput is 0.
	return m_rawinput ? m_rawinput->value != 0 : true;
}

#if defined(TS_INPUT_SDL)
// GoldSrc Linux (i386) owns the mouse grab CLIENT-side, exactly like valve's client.so:
// SDL relative mode confines + hides the cursor so mouse-look isn't bounded by the window
// (without it, yaw locks at the window edge — the ±90° bug). We release the grab whenever a
// VGUI panel needs the cursor (g_iVisibleMouse) and re-grab on return to gameplay. On Windows
// the OS is re-centered each frame instead (IN_ResetMouse); on Xash/wasm the engine owns the
// grab — so this whole mechanism is i386-Linux only.
static SDL_bool mouseRelative = SDL_TRUE;

static void IN_SetMouseRelative(bool enable)
{
	const SDL_bool value = enable ? SDL_TRUE : SDL_FALSE;

	SDL_SetRelativeMouseMode(value);
	mouseRelative = value;
}
#endif

// -----------------------------------------------------------------------------
// Engine-push input (Xash3D FWGS extension). The engine owns the mouse/joystick and
// PUSHES per-frame deltas into the client via IN_ClientLookEvent / IN_ClientMoveEvent,
// which it resolves by name (COM_GetProcAddress). This replaces the old client-side
// SDL_GetRelativeMouseState polling, so the client links no libSDL2. rel_yaw/rel_pitch
// arrive already scaled by the engine's m_yaw/m_pitch (raw_counts * m_yaw); the mouse
// pipeline in IN_MouseMove inverts that to recover the raw delta so TS's accel +
// sensitivity model is preserved exactly. BOTH exports must exist: the engine calls
// pfnMoveEvent unconditionally whenever pfnLookEvent is present.
static float s_relYaw = 0.0f;
static float s_relPitch = 0.0f;
static float s_acForward = 0.0f;
static float s_acSide = 0.0f;
// Set the first time the engine actually PUSHES input (Xash fork). Retail GoldSrc has no
// IN_ClientLookEvent/IN_ClientMoveEvent API and never calls these, so the flag stays false
// and IN_MouseMove falls back to polling the engine cursor (the classic HL SDK mouse loop).
static bool s_pushInputSeen = false;

extern "C" void DLLEXPORT IN_ClientLookEvent(float relyaw, float relpitch)
{
	s_pushInputSeen = true;
	s_relYaw += relyaw;
	s_relPitch += relpitch;
}

extern "C" void DLLEXPORT IN_ClientMoveEvent(float forwardmove, float sidemove)
{
	s_pushInputSeen = true;
	s_acForward += forwardmove;
	s_acSide += sidemove;
}

static bool m_bMouseThread = false;

// mouse variables
cvar_t* m_filter;
cvar_t* sensitivity;

// TS mouse acceleration — the original's own model (client.dll FUN_10028320 registers these
// three in the slot where the stock SDK registers m_customaccel*; it REPLACED that mechanism).
// Per axis: mx += sign(raw int delta) * min(pow(fabs(mx), m_accelexp) * m_accel, m_accelmax),
// m_accelmax 0 = no cap, then mx *= sensitivity. Zoomed: only the zoom sensitivity is applied.
static cvar_t* m_accel;
static cvar_t* m_accelexp;
static cvar_t* m_accelmax;

int mouse_buttons;
int mouse_oldbuttonstate;
int old_mouse_x, old_mouse_y, mx_accum, my_accum;
float mouse_x, mouse_y;

static bool restore_spi;
static int originalmouseparms[3], newmouseparms[3] = {0, 0, 1};
static bool mouseactive = false;
bool mouseinitialized;
static bool mouseparmsvalid;
static bool mouseshowtoggle = true;

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS 0x00000000 // control like a joystick
#define JOY_RELATIVE_AXIS 0x00000010 // control like a mouse, spinner, trackball
#define JOY_MAX_AXES 6				 // X, Y, Z, R, U, V
#define JOY_AXIS_X 0
#define JOY_AXIS_Y 1
#define JOY_AXIS_Z 2
#define JOY_AXIS_R 3
#define JOY_AXIS_U 4
#define JOY_AXIS_V 5

enum _ControlList
{
	AxisNada = 0,
	AxisForward,
	AxisLook,
	AxisSide,
	AxisTurn
};



std::uint32_t dwAxisMap[JOY_MAX_AXES];
std::uint32_t dwControlMap[JOY_MAX_AXES];
int pdwRawValue[JOY_MAX_AXES];
std::uint32_t joy_oldbuttonstate, joy_oldpovstate;

int joy_id;
std::uint32_t joy_numbuttons;

// Engine owns the joystick now (movement pushed via IN_ClientMoveEvent, buttons via
// the normal Key_Event path). Kept as an opaque handle for the inert legacy poll code.
void* s_pJoystick = nullptr;

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t* in_joystick;
cvar_t* joy_name;
cvar_t* joy_advanced;
cvar_t* joy_advaxisx;
cvar_t* joy_advaxisy;
cvar_t* joy_advaxisz;
cvar_t* joy_advaxisr;
cvar_t* joy_advaxisu;
cvar_t* joy_advaxisv;
cvar_t* joy_forwardthreshold;
cvar_t* joy_sidethreshold;
cvar_t* joy_pitchthreshold;
cvar_t* joy_yawthreshold;
cvar_t* joy_forwardsensitivity;
cvar_t* joy_sidesensitivity;
cvar_t* joy_pitchsensitivity;
cvar_t* joy_yawsensitivity;
cvar_t* joy_wwhack1;
cvar_t* joy_wwhack2;

bool joy_avail = false, joy_advancedinit, joy_haspov;

#ifdef WIN32
struct MouseThread
{
	std::thread Thread;
	std::mutex Mutex;
	std::condition_variable Condition;
	bool QuittingTime = false;
};

MouseThread s_MouseThread;

std::atomic<Point> s_mouseDelta;
std::atomic<Point> old_mouse_pos;

Point GetMousePosition()
{
	POINT mouse_pos;
	GetCursorPos(&mouse_pos);

	return {
		static_cast<int>(mouse_pos.x),
		static_cast<int>(mouse_pos.y)};
}

void MousePos_ThreadFunction()
{
	while (true)
	{
		{
			std::unique_lock lock{s_MouseThread.Mutex};

			// TS parity: m_mousethread_sleep removed (an HL25 cvar the original client never
			// registers) — the optional -mousethread path keeps its default 10 ms poll.
			if (s_MouseThread.Condition.wait_for(
					lock,
					std::chrono::milliseconds{10},
					[]()
					{ return s_MouseThread.QuittingTime; }))
			{
				break;
			}
		}

		if (mouseactive)
		{
			const auto mouse_pos = GetMousePosition();

			const auto oldPos = old_mouse_pos.load();
			const auto delta = s_mouseDelta.load();

			const Point pos{
				mouse_pos.x - oldPos.x + delta.x,
				mouse_pos.y - oldPos.y + delta.y};

			old_mouse_pos = mouse_pos;
			s_mouseDelta = pos;
		}
	}
}
#endif

/*
===========
IN_ActivateMouse
===========
*/
void DLLEXPORT IN_ActivateMouse()
{
	if (mouseinitialized)
	{
#ifdef WIN32
		if (mouseparmsvalid)
			restore_spi = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0) != FALSE;

#endif
		mouseactive = true;
	}

#if defined(TS_INPUT_SDL)
	// GoldSrc Linux: grab the cursor for mouse-look unless a VGUI panel needs it visible.
	// (Windows re-centers via IN_ResetMouse; Xash/wasm let the engine own the grab.)
	if (g_iVisibleMouse || !IN_UseRawInput())
	{
		IN_SetMouseRelative(false);
	}
	else
	{
		IN_SetMouseRelative(true);
	}
#endif

	// Clear out accumulated mouse input from main menu movement.
	IN_ResetMouse();
}


/*
===========
IN_DeactivateMouse
===========
*/
void DLLEXPORT IN_DeactivateMouse()
{
	if (mouseinitialized)
	{
#ifdef WIN32
		if (restore_spi)
			SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

#endif

		mouseactive = false;
	}

#if defined(TS_INPUT_SDL)
	// GoldSrc Linux: release the cursor grab when the mouse is deactivated.
	IN_SetMouseRelative(false);
#endif
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse()
{
	if (0 != gEngfuncs.CheckParm("-nomouse", NULL))
		return;

	mouseinitialized = true;
#ifdef WIN32
	mouseparmsvalid = SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0) != FALSE;

	if (mouseparmsvalid)
	{
		if (0 != gEngfuncs.CheckParm("-noforcemspd", NULL))
			newmouseparms[2] = originalmouseparms[2];

		if (0 != gEngfuncs.CheckParm("-noforcemaccel", NULL))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
		}

		if (0 != gEngfuncs.CheckParm("-noforcemparms", NULL))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
			newmouseparms[2] = originalmouseparms[2];
		}
	}
#endif

	mouse_buttons = MOUSE_BUTTON_COUNT;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown()
{
	IN_DeactivateMouse();

#ifdef WIN32
	if (s_MouseThread.Thread.joinable())
	{
		//Mouse thread is active, signal it to quit and wait.
		{
			std::lock_guard guard{s_MouseThread.Mutex};
			s_MouseThread.QuittingTime = true;
		}

		s_MouseThread.Condition.notify_one();
		s_MouseThread.Thread.join();
	}
#endif
}

/*
===========
IN_GetMousePos

Ask for mouse position from engine
===========
*/
void IN_GetMousePos(int* mx, int* my)
{
	gEngfuncs.GetMousePosition(mx, my);
}

/*
===========
IN_GetMouseSensitivity

Get mouse sensitivity with sanitization
===========
*/
float IN_GetMouseSensitivity()
{
	// Absurdly high sensitivity values can cause the game to hang, so clamp
	if (sensitivity->value > 10000.0)
	{
		gEngfuncs.Cvar_SetValue("sensitivity", 10000.0);
	}
	else if (sensitivity->value < 0.01)
	{
		gEngfuncs.Cvar_SetValue("sensitivity", 0.01);
	}

	return sensitivity->value;
}

/*
===========
IN_ResetMouse

FIXME: Call through to engine?
===========
*/
void IN_ResetMouse()
{
	// SDL relative mode (Linux) and the engine-push path (Xash/wasm) auto-reset — nothing to do.
#ifdef WIN32
	// The Win32 GetCursorPos path reads absolute position, so re-center every frame. That path
	// runs whenever the engine isn't pushing (i.e. GoldSrc Windows), regardless of m_rawinput.
	if (!s_pushInputSeen && mouseactive && gEngfuncs.GetWindowCenterX && gEngfuncs.GetWindowCenterY)
	{
		SetCursorPos(gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());

		const Point center{gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY()};
		old_mouse_pos = center;
	}
#endif
}

/*
===========
IN_ResetRelativeMouseState
===========
*/
void IN_ResetRelativeMouseState(void)
{
	// Discard any pending delta so it doesn't leak across a state change (menu <-> game).
	s_relYaw = 0.0f;    // engine-push (Xash/wasm)
	s_relPitch = 0.0f;
#if defined(TS_INPUT_SDL)
	// GoldSrc Linux: flush SDL's accumulated relative motion so it doesn't jump the view.
	SDL_PumpEvents();
	int deltaX, deltaY;
	SDL_GetRelativeMouseState(&deltaX, &deltaY);
	(void)deltaX;
	(void)deltaY;
#endif
}

/*
===========
IN_MouseEvent
===========
*/
void DLLEXPORT IN_MouseEvent(int mstate)
{
	int i;

	// client.dll 0x100273d0: with the cursor hidden every button change is a key event. With it shown
	// (a VGUI window or the TS buy menu is up) a click goes to the buy menu (left = an entry or, off
	// every entry, key 1 "use config", which closes it; right = Go Back; FUN_10038360) only while no
	// VGUI window is up, and otherwise to VGUI alone. So at a join the MOTD takes the clicks until it is
	// closed, then the buy menu, and the click that closes the buy menu gives the mouse back to the game.
	// (reTS gave the buy menu every click first, so a click closed it under the MOTD, whose cursor then
	// stayed with the mouse never captured again.)
	if (iMouseInUse || g_iVisibleMouse)
	{
		if (gHUD.m_TSBuyMenu.IsUp() && !TS_VGUIMenuUp())
			gHUD.m_TSBuyMenu.HandleMouse(mstate);

		// Keep the edge-detect state in sync while the cursor is up. Otherwise mouse_oldbuttonstate stays
		// frozen, and once the cursor goes the next IN_MouseEvent compares the live button to that stale
		// state and synthesizes a spurious Key_Event(K_MOUSE1) = +attack DOWN, which fires the freshly
		// spawned weapon (the "buy-menu click -> fire on spawn" bug). A click the menu or VGUI consumed
		// must not leak.
		mouse_oldbuttonstate = mstate;
		return;
	}

	// perform button actions
	for (i = 0; i < mouse_buttons; i++)
	{
		if ((mstate & (1 << i)) != 0 &&
			(mouse_oldbuttonstate & (1 << i)) == 0)
		{
			gEngfuncs.Key_Event(K_MOUSE1 + i, 1);
		}

		if ((mstate & (1 << i)) == 0 &&
			(mouse_oldbuttonstate & (1 << i)) != 0)
		{
			gEngfuncs.Key_Event(K_MOUSE1 + i, 0);
		}
	}

	mouse_oldbuttonstate = mstate;
}

//-----------------------------------------------------------------------------
// Purpose: Mouse scaling/acceleration — the ORIGINAL TS model (client.dll
//  IN_MouseMove = FUN_100274d0). Zoomed (gHUD.GetSensitivity() != 0): scale by
//  the zoom sensitivity ONLY — no accel, no sensitivity cvar. Not zoomed: per
//  axis ADDITIVE accel — fabs of the FILTERED float feeds pow, the SIGN comes
//  from the raw int delta, m_accelmax clamps only when non-zero — then the
//  sensitivity cvar is applied AFTER accel.
// Input  : ix, iy - the raw (unfiltered) int mouse deltas
//			*x, *y - the filtered float deltas, scaled in place
//-----------------------------------------------------------------------------
void IN_ScaleMouse(int ix, int iy, float* x, float* y)
{
	float zoomSens = gHUD.GetSensitivity();
	if (zoomSens != 0)
	{
		*x *= zoomSens;
		*y *= zoomSens;
		return;
	}

	float ax = powf(fabsf(*x), m_accelexp->value) * m_accel->value;
	if (m_accelmax->value != 0 && m_accelmax->value <= ax)
		ax = m_accelmax->value;
	*x += (ix > 0 ? 1.0f : -1.0f) * ax;

	float ay = powf(fabsf(*y), m_accelexp->value) * m_accel->value;
	if (m_accelmax->value != 0 && m_accelmax->value <= ay)
		ay = m_accelmax->value;
	*y += (iy > 0 ? 1.0f : -1.0f) * ay;

	*x *= sensitivity->value;
	*y *= sensitivity->value;
}

/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove(float frametime, usercmd_t* cmd)
{
	Point pos;
	Vector viewangles;

	gEngfuncs.GetViewAngles((float*)viewangles);

	if ((in_mlook.state & 1) != 0)
	{
		V_StopPitchDrift();
	}

	//jjb - this disbles normal mouse control if the user is trying to
	//      move the camera, or if the mouse cursor is visible or if we're in intermission
	if (!iMouseInUse && !gHUD.m_iIntermission && !g_iVisibleMouse)
	{
		int deltaX, deltaY;
		if (s_pushInputSeen)
		{
			// Xash push (wasm + native Xash): the engine already scaled raw counts by
			// m_yaw/m_pitch; invert to recover the raw delta so TS's accel/sensitivity model
			// (below) applies exactly. Guard the divide (m_yaw/m_pitch can be 0).
			deltaX = (m_yaw->value != 0.0f) ? (int)lroundf(-s_relYaw / m_yaw->value) : 0;
			deltaY = (m_pitch->value != 0.0f) ? (int)lroundf(s_relPitch / m_pitch->value) : 0;
			s_relYaw = 0.0f;
			s_relPitch = 0.0f;
		}
		else
		{
#if defined(_WIN32)
			// GoldSrc Windows: classic GetCursorPos, delta from window center; IN_ResetMouse()
			// re-centers each frame (SetCursorPos). Identical to the original ts client.dll.
			if (m_bMouseThread)
			{
				const Point d = s_mouseDelta.load();
				s_mouseDelta = Point{};
				deltaX = d.x;
				deltaY = d.y;
			}
			else
			{
				const Point p = GetMousePosition();
				deltaX = p.x - gEngfuncs.GetWindowCenterX();
				deltaY = p.y - gEngfuncs.GetWindowCenterY();
			}
#elif defined(TS_INPUT_SDL)
			// GoldSrc Linux (i386): SDL relative mouse — the only native path, matches valve's
			// client.so. The engine holds SDL in relative mode while the mouse is grabbed; we
			// just drain the accumulated relative delta (no re-centering needed).
			SDL_GetRelativeMouseState(&deltaX, &deltaY);
#else
			// amd64/aarch64 (Xash native) + wasm: input only ever arrives via the push above,
			// so this fallback is unreachable — kept SDL-free (no libSDL2 on those targets).
			deltaX = 0;
			deltaY = 0;
#endif
		}

		pos.x = deltaX + mx_accum;
		pos.y = deltaY + my_accum;
		mx_accum = 0;
		my_accum = 0;

		if (m_filter && 0 != m_filter->value)
		{
			mouse_x = (pos.x + old_mouse_x) * 0.5;
			mouse_y = (pos.y + old_mouse_y) * 0.5;
		}
		else
		{
			mouse_x = pos.x;
			mouse_y = pos.y;
		}

		old_mouse_x = pos.x;
		old_mouse_y = pos.y;

		// Apply mouse scaling/acceleration (the raw int deltas carry the sign for the accel term)
		IN_ScaleMouse(pos.x, pos.y, &mouse_x, &mouse_y);

		// add mouse X/Y movement to cmd: client.dll's IN_MouseMove (0x100277c0) always yaws by the mouse
		// (it has no +strafe or +mlook commands and never reads m_side) and pitches unless in_strafe is
		// down, which nothing can press
		viewangles[YAW] -= m_yaw->value * mouse_x;

		if ((in_strafe.state & 1) == 0)
		{
			viewangles[PITCH] += m_pitch->value * mouse_y;
			// TS parity: HARDCODED +/-89 — the original registers no cl_pitchup/cl_pitchdown
			// (client.dll FUN_100274d0 clamps to the literals).
			if (viewangles[PITCH] > 89.0f)
				viewangles[PITCH] = 89.0f;
			if (viewangles[PITCH] < -89.0f)
				viewangles[PITCH] = -89.0f;
		}
		else
		{
			if ((in_strafe.state & 1) != 0 && 0 != gEngfuncs.IsNoClipping())
			{
				cmd->upmove -= m_forward->value * mouse_y;
			}
			else
			{
				cmd->forwardmove -= m_forward->value * mouse_y;
			}
		}

		// if the mouse has moved, force it to the center, so there's room to move
		if (0 != pos.x || 0 != pos.y)
		{
			IN_ResetMouse();
		}
	}

	gEngfuncs.SetViewAngles((float*)viewangles);

#if defined(TS_INPUT_SDL)
	// GoldSrc Linux, per-frame: release the grab the moment a VGUI panel raises the cursor
	// (g_iVisibleMouse — set by vgui_TeamFortressViewport for the MOTD/buy menu/scoreboard) and
	// re-grab as soon as it's gone. This is the fix for "menu cursor doesn't appear / can't move
	// it": without it the grab stays on and the OS cursor is pinned to center under the panel.
	if ((!IN_UseRawInput() && SDL_FALSE != mouseRelative) || g_iVisibleMouse)
	{
		IN_SetMouseRelative(false);
	}
	else if (IN_UseRawInput() && SDL_FALSE == mouseRelative)
	{
		IN_SetMouseRelative(true);
	}
#endif

	/*
//#define TRACE_TEST
#if defined( TRACE_TEST )
	{
		int mx, my;
		void V_Move( int mx, int my );
		IN_GetMousePos( &mx, &my );
		V_Move( mx, my );
	}
#endif
*/
}

/*
===========
IN_Accumulate
===========
*/
void DLLEXPORT IN_Accumulate()
{
	//only accumulate mouse if we are not moving the camera with the mouse
	if (!iMouseInUse && !g_iVisibleMouse)
	{
		if (mouseactive)
		{
			if (s_pushInputSeen)
			{
				// Engine-push (Xash/wasm): the engine accumulates between frames via repeated
				// IN_ClientLookEvent calls; IN_MouseMove drains it. Nothing to poll here.
			}
			else
			{
#if defined(_WIN32)
				// GoldSrc Windows: accumulate the GetCursorPos delta from window center.
				if (!m_bMouseThread)
				{
					const Point p = GetMousePosition();
					mx_accum += p.x - gEngfuncs.GetWindowCenterX();
					my_accum += p.y - gEngfuncs.GetWindowCenterY();
				}
#elif defined(TS_INPUT_SDL)
				// GoldSrc Linux (i386): drain SDL's relative delta into the accumulator.
				int deltaX, deltaY;
				SDL_GetRelativeMouseState(&deltaX, &deltaY);
				mx_accum += deltaX;
				my_accum += deltaY;
#endif
			}
			// force the mouse to the center, so there's room to move
			IN_ResetMouse();
		}
	}
}

/*
===================
IN_ClearStates
===================
*/
void DLLEXPORT IN_ClearStates()
{
	if (!mouseactive)
		return;

	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}

/* 
=============== 
IN_StartupJoystick 
=============== 
*/
void IN_StartupJoystick()
{
	// abort startup if user requests no joystick
	if (0 != gEngfuncs.CheckParm("-nojoy", NULL))
		return;

	static float flLastCheck = 0.0f;
	if (flLastCheck > 0.0f && (gEngfuncs.GetAbsoluteTime() - flLastCheck) < 1.0f)
		return;

	//gEngfuncs.Con_Printf("IN_StartupJoystick, %f\n", flLastCheck);

	flLastCheck = gEngfuncs.GetAbsoluteTime();

	// No client-side device enumeration: the engine owns the joystick and pushes
	// movement via IN_ClientMoveEvent (applied in IN_Move); buttons arrive through the
	// engine's normal Key_Event path. Mark the legacy poll path inert.
	s_pJoystick = nullptr;
	joy_avail = 0;
}


int RawValuePointer(int axis)
{
	// Legacy client-side axis read is retired (the engine pushes movement via
	// IN_ClientMoveEvent). Inert: joy_avail is always false so this is never reached.
	(void)axis;
	return 0;
}

/*
===========
Joy_AdvancedUpdate_f
===========
*/
void Joy_AdvancedUpdate_f()
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int i;
	std::uint32_t dwTemp;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if (joy_advanced->value == 0.0)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (strcmp(joy_name->string, "joystick") != 0)
		{
			// notify user of advanced controller
			gEngfuncs.Con_Printf("\n%s configured\n\n", joy_name->string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (std::uint32_t)joy_advaxisx->value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisy->value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisz->value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisr->value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisu->value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (std::uint32_t)joy_advaxisv->value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}
}


/*
===========
IN_Commands
===========
*/
void IN_Commands()
{
	int i, key_index;

	if (!joy_avail)
	{
		return;
	}

	std::uint32_t buttonstate, povstate;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	// Joystick buttons arrive via the engine's normal Key_Event path now (no SDL poll).
	buttonstate = 0;

	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		pdwRawValue[i] = RawValuePointer(i);
	}

	for (i = 0; i < (int)joy_numbuttons; i++)
	{
		if ((buttonstate & (1 << i)) != 0 && (joy_oldbuttonstate & (1 << i)) == 0)
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			gEngfuncs.Key_Event(key_index + i, 1);
			//gEngfuncs.Con_Printf("Button %d pressed\n", i);
		}

		if ((buttonstate & (1 << i)) == 0 && (joy_oldbuttonstate & (1 << i)) != 0)
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			gEngfuncs.Key_Event(key_index + i, 0);
			//gEngfuncs.Con_Printf("Button %d released\n", i);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		// determine which bits have changed and key an auxillary event for each change
		for (i = 0; i < 4; i++)
		{
			if ((povstate & (1 << i)) != 0 && (joy_oldpovstate & (1 << i)) == 0)
			{
				gEngfuncs.Key_Event(K_AUX29 + i, 1);
			}

			if ((povstate & (1 << i)) == 0 && (joy_oldpovstate & (1 << i)) != 0)
			{
				gEngfuncs.Key_Event(K_AUX29 + i, 0);
			}
		}
		joy_oldpovstate = povstate;
	}
}


/* 
=============== 
IN_ReadJoystick
=============== 
*/
bool IN_ReadJoystick()
{
	// Engine owns joystick polling now; nothing to update client-side.
	return true;
}


/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove(float frametime, usercmd_t* cmd)
{
	float speed, aspeed;
	float fAxisValue, fTemp;
	int i;
	Vector viewangles;

	gEngfuncs.GetViewAngles((float*)viewangles);


	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (!joy_advancedinit)
	{
		Joy_AdvancedUpdate_f();
		joy_advancedinit = true;
	}

	// re-scan for joystick presence
	IN_StartupJoystick();

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || 0 == in_joystick->value)
	{
		return;
	}

	// collect the joystick data, if possible
	if (!IN_ReadJoystick())
	{
		return;
	}

	if ((in_speed.state & 1) != 0)
		speed = cl_movespeedkey->value;
	else
		speed = 1;

	aspeed = speed * frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float)pdwRawValue[i];

		if (joy_wwhack2->value != 0.0)
		{
			if (dwAxisMap[i] == AxisTurn)
			{
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(fabs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i])
		{
		case AxisForward:
			if ((joy_advanced->value == 0.0) && (in_jlook.state & 1) != 0)
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold->value)
				{
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is 0)
					if (m_pitch->value < 0.0)
					{
						viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					}
					else
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					}
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (lookspring->value == 0.0)
					{
						V_StopPitchDrift();
					}
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold->value)
				{
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity->value) * speed * cl_forwardspeed->value;
				}
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold->value)
			{
				cmd->sidemove += (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
			}
			break;

		case AxisTurn:
			if ((in_strafe.state & 1) != 0 || (0 != lookstrafe->value && (in_jlook.state & 1) != 0))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold->value)
				{
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
				}
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold->value)
				{
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
					{
						viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
					}
					else
					{
						viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * speed * 180.0;
					}
				}
			}
			break;

		case AxisLook:
			if ((in_jlook.state & 1) != 0)
			{
				if (fabs(fAxisValue) > joy_pitchthreshold->value)
				{
					// pitch movement detected and pitch movement desired by user
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					}
					else
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * speed * 180.0;
					}
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (lookspring->value == 0.0)
					{
						V_StopPitchDrift();
					}
				}
			}
			break;

		default:
			break;
		}
	}

	// bounds check pitch — hardcoded +/-89 (no cl_pitchup/cl_pitchdown in the original)
	if (viewangles[PITCH] > 89.0f)
		viewangles[PITCH] = 89.0f;
	if (viewangles[PITCH] < -89.0f)
		viewangles[PITCH] = -89.0f;

	gEngfuncs.SetViewAngles((float*)viewangles);
}

/*
===========
IN_Move
===========
*/
void IN_Move(float frametime, usercmd_t* cmd)
{
	if (!iMouseInUse && mouseactive)
	{
		IN_MouseMove(frametime, cmd);
	}

	IN_JoyMove(frametime, cmd);

	// Engine-push movement (gamepad/touch the engine owns, via IN_ClientMoveEvent) —
	// replaces the old SDL gamecontroller axes. Zero for mouse+keyboard players, so this
	// is a no-op for the common case and adds gamepad support where the engine provides it.
	cmd->forwardmove += s_acForward;
	cmd->sidemove += s_acSide;
	s_acForward = 0.0f;
	s_acSide = 0.0f;
}

/*
===========
IN_Init
===========
*/
void IN_Init()
{
	m_filter = gEngfuncs.pfnRegisterVariable("m_filter", "0", FCVAR_ARCHIVE);
	sensitivity = gEngfuncs.pfnRegisterVariable("sensitivity", "3", FCVAR_ARCHIVE); // user mouse sensitivity setting (client.dll 0x10028343: ARCHIVE only)

	// TS mouse accel — registered here in the original (client.dll FUN_10028320, right after
	// m_filter/sensitivity, before the joystick block; replaces the stock m_customaccel*).
	m_accel = gEngfuncs.pfnRegisterVariable("m_accel", "0", FCVAR_ARCHIVE);
	m_accelexp = gEngfuncs.pfnRegisterVariable("m_accelexp", "1.2", FCVAR_ARCHIVE);
	m_accelmax = gEngfuncs.pfnRegisterVariable("m_accelmax", "0", FCVAR_ARCHIVE);

	in_joystick = gEngfuncs.pfnRegisterVariable("joystick", "0", FCVAR_ARCHIVE);
	joy_name = gEngfuncs.pfnRegisterVariable("joyname", "joystick", 0);
	joy_advanced = gEngfuncs.pfnRegisterVariable("joyadvanced", "0", 0);
	joy_advaxisx = gEngfuncs.pfnRegisterVariable("joyadvaxisx", "0", 0);
	joy_advaxisy = gEngfuncs.pfnRegisterVariable("joyadvaxisy", "0", 0);
	joy_advaxisz = gEngfuncs.pfnRegisterVariable("joyadvaxisz", "0", 0);
	joy_advaxisr = gEngfuncs.pfnRegisterVariable("joyadvaxisr", "0", 0);
	joy_advaxisu = gEngfuncs.pfnRegisterVariable("joyadvaxisu", "0", 0);
	joy_advaxisv = gEngfuncs.pfnRegisterVariable("joyadvaxisv", "0", 0);
	joy_forwardthreshold = gEngfuncs.pfnRegisterVariable("joyforwardthreshold", "0.15", 0);
	joy_sidethreshold = gEngfuncs.pfnRegisterVariable("joysidethreshold", "0.15", 0);
	joy_pitchthreshold = gEngfuncs.pfnRegisterVariable("joypitchthreshold", "0.15", 0);
	joy_yawthreshold = gEngfuncs.pfnRegisterVariable("joyyawthreshold", "0.15", 0);
	joy_forwardsensitivity = gEngfuncs.pfnRegisterVariable("joyforwardsensitivity", "-1.0", 0);
	joy_sidesensitivity = gEngfuncs.pfnRegisterVariable("joysidesensitivity", "-1.0", 0);
	joy_pitchsensitivity = gEngfuncs.pfnRegisterVariable("joypitchsensitivity", "1.0", 0);
	joy_yawsensitivity = gEngfuncs.pfnRegisterVariable("joyyawsensitivity", "-1.0", 0);
	joy_wwhack1 = gEngfuncs.pfnRegisterVariable("joywwhack1", "0.0", 0);
	joy_wwhack2 = gEngfuncs.pfnRegisterVariable("joywwhack2", "0.0", 0);

#ifdef WIN32
	m_rawinput = gEngfuncs.pfnGetCvarPointer("m_rawinput");
	m_bMouseThread = gEngfuncs.CheckParm("-mousethread", NULL) != NULL;

	if (!IN_UseRawInput() && m_bMouseThread)
	{
		s_mouseDelta = Point{};

		s_MouseThread.Thread = std::thread{&MousePos_ThreadFunction};
	}
#endif

	gEngfuncs.pfnAddCommand("joyadvancedupdate", Joy_AdvancedUpdate_f);

	// TS is an always-mouselook game (no legacy keyboard-look mode). The stock HL
	// client leaves mouse-look OFF until a config issues "+mlook"; when it's off the
	// mouse Y axis walks the player forward/back and can't pitch, which reads as a
	// locked, jolting view. Force it on so looking works out of the box.
	in_mlook.state |= 1;

	IN_StartupMouse();
	IN_StartupJoystick();
}
