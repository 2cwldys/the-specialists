// rets_loadout_probe: what a match plugin's loadout really gave each bot, read from the game's own
// weapon-bar message (WStatus: BYTE type, BYTE owned, BYTE clip), which the game sends for every slot
// that changes. At localinfo "ms_delay" seconds (default 25) it forces the match live through the
// plugin's console command (localinfo "ms_cmd", default amx_match_start); 12 s and 30 s later it prints
// "[ld] <name>: slots <n>, owns <type>x<clip> ..." for each live bot, then "[ld] done". Run by
// scripts/test-win-server.sh with ADDONS=<suite zip> PROBE=rets_loadout_probe EXTRA_SMA=<the plugin>
// RCBOT=1.
#include <amxmodx>
#include <tsfun>

#define TS_WEAPONS	40

new g_owned[33][TS_WEAPONS]
new g_clip[33][TS_WEAPONS]
new g_type, g_who
new g_report

public plugin_init()
{
	register_plugin("rets_loadout_probe", "1", "reTS")
	register_message(get_user_msgid("WStatus"), "msg_wstatus")
	new d[8]
	get_localinfo("ms_delay", d, charsmax(d))
	new Float:delay = d[0] ? str_to_float(d) : 25.0
	set_task(delay, "start_match")
	set_task(delay + 12.0, "report")
	set_task(delay + 30.0, "report")
}

public client_putinserver(id)
{
	for (new w = 0; w < TS_WEAPONS; w++)
		g_owned[id][w] = g_clip[id][w] = 0
}

public msg_wstatus(msgid, dest, id)
{
	if (id < 1 || id > 32)
		return PLUGIN_CONTINUE
	g_who = id
	g_type = get_msg_arg_int(1)
	if (g_type >= 0 && g_type < TS_WEAPONS)
	{
		g_owned[g_who][g_type] = get_msg_arg_int(2)
		g_clip[g_who][g_type] = get_msg_arg_int(3)
	}
	return PLUGIN_CONTINUE
}

public start_match()
{
	new cmd[64]
	get_localinfo("ms_cmd", cmd, charsmax(cmd))
	if (!cmd[0])
		copy(cmd, charsmax(cmd), "amx_match_start")
	server_print("[ld] players %d, sending '%s'", get_playersnum(1), cmd)
	server_cmd(cmd)
}

public report()
{
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id) || !is_user_bot(id) || !is_user_alive(id))
			continue
		new name[32], line[160], len
		get_user_name(id, name, charsmax(name))
		for (new w = 1; w < TS_WEAPONS; w++)
			if (g_owned[id][w])
				len += formatex(line[len], charsmax(line) - len, " %dx%d", w, g_clip[id][w])
		server_print("[ld] %s: slots %d, owns%s", name, ts_getuserslots(id), len ? line : " nothing")
	}
	if (++g_report == 2)
		server_print("[ld] done")
}
