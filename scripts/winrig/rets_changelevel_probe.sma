// rets_changelevel_probe: exercise AMX Mod X core's engine patches (the SV_DropClient detour, and the
// Cvar_DirectSet one behind cvar hooks): add a fake client, kick it (SV_DropClient), hook a cvar and set
// it, then change level twice (ServerDeactivate removes the SV_DropClient detour). Prints "[cl] map <n>"
// on each map and "[cl] kicked" after the kick; a server that dies shows where it stopped. Run by
// scripts/test-win-server.sh with ADDONS=<suite zip> PROBE=rets_changelevel_probe.
#include <amxmodx>
#include <fakemeta>

new g_bot

public plugin_init()
{
	register_plugin("rets_changelevel_probe", "1", "reTS")
	new n[8]
	get_localinfo("clprobe_map", n, charsmax(n))
	new map = str_to_num(n) + 1
	num_to_str(map, n, charsmax(n))
	set_localinfo("clprobe_map", n)
	server_print("[cl] map %d", map)
	hook_cvar_change(get_cvar_pointer("mp_friendlyfire"), "on_ff")
	if (map <= 3)
		set_task(3.0, "make_bot")
}

public on_ff(pcvar, const oldv[], const newv[]) { server_print("[cl] cvar hook fired %s -> %s", oldv, newv); }

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "clprobe")
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "clprobe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	set_task(2.0, "kick_bot")
}

public kick_bot()
{
	server_cmd("kick #%d", get_user_userid(g_bot))
	server_exec()
	server_print("[cl] kicked")
	set_cvar_num("mp_friendlyfire", get_cvar_num("mp_friendlyfire") ? 0 : 1)
	set_task(2.0, "next_map")
}

public next_map()
{
	server_print("[cl] changelevel")
	server_cmd("changelevel ts_hammertime")
}

public client_disconnected(id) { server_print("[cl] client_disconnected %d", id); }
