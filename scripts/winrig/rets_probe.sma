// rets_probe: drive stock AMX Mod X 1.8.2's TS modules (tsx, tsfun) against the running game library,
// the way a mode plugin does, and log what reads back. Run by scripts/test-win-server.sh with ADDONS=1.
// On TS 3.0 the module's offsets are right; on reTS they are not (reference/DEVIATIONS.md), so this shows
// what a stock install does to a reTS server.
#include <amxmodx>
#include <fakemeta>
#include <tsx>
#include <tsfun>

new g_bot
new g_tick

public plugin_init()
{
	register_plugin("rets_probe", "1", "reTS")
	set_task(3.0, "make_bot")
}

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "probe")
	if (!g_bot) { server_print("[probe] no fake client"); return; }
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "probe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	server_print("[probe] fake client %d in server", g_bot)
	set_task(1.0, "tick", 0, "", 0, "b")
}

public tick()
{
	if (!is_user_connected(g_bot)) { server_print("[probe] bot gone"); return; }
	g_tick++
	new cash0 = ts_getusercash(g_bot)
	new slots0 = ts_getuserslots(g_bot)
	ts_setusercash(g_bot, 1000 + g_tick)
	ts_setuserslots(g_bot, 40)
	if (g_tick % 5 == 0)
		ts_giveweapon(g_bot, 5, 3, 0) // M4A1, as a mode plugin's loadout would
	if (g_tick % 7 == 0)
		ts_set_speed(g_bot, 0.5, 0.0, 2.0)
	new clip, ammo, mode, extra
	new wpn = ts_getuserwpn(g_bot, clip, ammo, mode, extra)
	server_print("[probe] tick %d cash %d->%d slots %d->%d wpn %d clip %d ammo %d health %d alive %d",
		g_tick, cash0, ts_getusercash(g_bot), slots0, ts_getuserslots(g_bot), wpn, clip, ammo,
		get_user_health(g_bot), is_user_alive(g_bot))
}
